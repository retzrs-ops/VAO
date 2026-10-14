// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

// 測定: 無音 → 定型文 → 一番大きい声 の 3 段階で入力を測る。OBS に依存しない。
// feed() だけが音声スレッドから呼ばれる（確保・ロック・ログなし）。
// それ以外は UI 側のスレッドから呼ぶ。

#pragma once

#include <atomic>
#include <cstddef>
#include <vector>

namespace vao::calib {

struct Measurement {
	float noise_floor_db = -90.0f; // 部屋のノイズ（無音時の平均、dBFS）
	float speech_level_db = -90.0f; // 声の平均（しゃべっている部分、dBFS の二乗平均）
	float peak_db = -90.0f;        // 声と大声の最大ピーク（dBFS）
	bool clipped = false;          // 入力の時点で音割れしている
};

class Measurer {
public:
	enum class Phase : int { Idle, Starting, Silence, Speech, Loud, Done, Failed };

	// 各段階の長さ（秒）。画面の案内にも使う。
	static constexpr int kSilenceSec = 3;
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
	static constexpr float kBlockSec = 0.05f;
	static constexpr int kSilenceSkipBlocks = 10; // 最初の 0.5 秒は使わない
	static constexpr int kActiveMarginDb = 10;
	static constexpr int kMinActiveBlocks = 20; // 声が 1 秒分ない場合は失敗

	int blocks_in(Phase p) const;
	bool advance(Phase from, Phase to);

	std::atomic<Phase> phase_{Phase::Idle};
	std::atomic<int> block_in_phase_{0};
	float sample_rate_ = 48000.0f;
	std::size_t block_len_ = 2400;

	// 以下は音声スレッドが書く（Done になるまで UI 側は読まない）
	std::size_t frames_in_block_ = 0;
	double sum_sq_ = 0.0;
	float block_peak_ = 0.0f;
	int clip_run_ = 0;
	bool clipped_ = false;
	std::vector<float> power_; // ブロックごとの二乗平均
	std::vector<float> peak_;
	std::size_t stored_ = 0;
};

} // namespace vao::calib
