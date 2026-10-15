// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

// 測定: 無音 → キーボード → マウス → 定型文 → 一番大きい声 の 5 段階で入力を測る。OBS に依存しない。
// feed() だけが音声スレッドから呼ばれる（確保・ロック・ログなし）。
// それ以外は UI 側のスレッドから呼ぶ。

#pragma once

#include <atomic>
#include <cstddef>
#include <vector>

#include "dsp/biquad.h"

namespace vao::calib {

struct Measurement {
	float noise_floor_db = -90.0f;    // 部屋のノイズ（無音時の平均、dBFS の二乗平均）
	float keyboard_level_db = -90.0f; // キーボードを叩く音の最大の強さ（10 ms の二乗平均、dB。ゲートと同じ測り方）
	float mouse_level_db = -90.0f;    // マウスをクリックする音の最大の強さ（同上）
	float speech_level_db = -90.0f;   // 声の平均（しゃべっている部分、dBFS の二乗平均）
	float speech_lufs =
		-90.0f;         // 声の平均のラウドネス（LUFS。K 特性・チャンネルを足し合わせ・無音を除く簡易ゲート）
	float peak_db = -90.0f; // 声と大声の最大ピーク（dBFS）
	bool clipped = false;   // 入力の時点で音割れしている

	// 声の帯域ごとの強さ（しゃべっている部分の二乗平均、dB）
	float band_low_db = -90.0f;  // 100 Hz 付近
	float band_mud_db = -90.0f;  // 300 Hz 付近（こもり）
	float band_pres_db = -90.0f; // 3 kHz 付近（明瞭さ）
	float band_sib_db = -90.0f;  // 7 kHz 付近（サ行。ディエッサーと同じ帯域）
};

class Measurer {
public:
	enum class Phase : int { Idle, Starting, Silence, Keyboard, Mouse, Speech, Loud, Done, Failed };

	// 各段階の長さ（秒）。画面の案内にも使う。
	static constexpr int kSilenceSec = 3;
	static constexpr int kKeyboardSec = 4;
	static constexpr int kMouseSec = 3;
	static constexpr int kSpeechSec = 12;
	static constexpr int kLoudSec = 4;

	void prepare(float sample_rate); // バッファを確保する。音声スレッドでは呼ばない

	void start();  // 測定を始める（やり直しも可）
	void cancel(); // 測定をやめる

	Phase phase() const { return phase_.load(std::memory_order_acquire); }
	int remaining_sec() const; // 現在の段階の残り秒数（切り上げ）

	// 音声スレッド。nullptr のチャンネルは飛ばす。
	void feed(const float *const *channels, std::size_t num_channels, std::size_t num_frames);

	// phase() が Done になったあとに呼ぶ。声が小さすぎて測れなければ false（Failed になる）。
	bool compute(Measurement &out);

private:
	static constexpr int kBands = 4;
	static constexpr float kBlockSec = 0.05f;
	static constexpr int kSilenceSkipBlocks = 10; // 最初の 0.5 秒は使わない
	static constexpr int kActiveMarginDb = 10;
	static constexpr int kMinActiveBlocks = 20; // 声が 1 秒分ない場合は失敗

	int blocks_in(Phase p) const;
	bool advance(Phase from, Phase to);

	std::atomic<Phase> phase_{Phase::Idle};
	std::atomic<int> block_in_phase_{0};
	float sample_rate_ = 48000.0f;
	double det_k_ = 0.0;
	std::size_t block_len_ = 2400;

	// 以下は音声スレッドが書く（Done になるまで UI 側は読まない）
	std::size_t frames_in_block_ = 0;
	double sum_sq_ = 0.0;
	double band_sq_[kBands] = {};
	float block_peak_ = 0.0f;
	int clip_run_ = 0;
	bool clipped_ = false;
	vao::dsp::Biquad band_filter_[kBands];
	vao::dsp::Biquad k_pre_[vao::dsp::kBiquadMaxChannels]; // K 特性: ① 高域を持ち上げる
	vao::dsp::Biquad k_rlb_[vao::dsp::kBiquadMaxChannels]; // K 特性: ② 低域を切る
	double k_sq_ = 0.0;                                    // ブロック内の K 特性の二乗の合計（チャンネルは足す）
	std::vector<float> power_;                             // ブロックごとの二乗平均
	std::vector<float> peak_;
	std::vector<float> kpower_;     // ブロックごとの K 特性の二乗平均（チャンネルの合計）
	double det_power_ = 0.0;        // 10 ms の二乗平均（ゲートと同じ検出）
	double click_max_[2] = {};      // キーボード・マウスの段階での検出値の最大
	std::vector<float> band_power_; // ブロックごと・帯域ごとの二乗平均（kBands 個ずつ）
	std::size_t stored_ = 0;
};

} // namespace vao::calib
