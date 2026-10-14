// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

// 測定結果から各フィルターの設定値を決める。OBS に依存しない純粋な計算（同じ入力なら同じ結果）。
// 数値は暫定。実機で調整する。

#pragma once

#include "dsp/eq.h"
#include "measurer.h"

namespace vao::calib {

constexpr float kTargetSpeechLevelDb = -16.0f; // 声の平均の目標
constexpr float kLimiterCeilingDb = -1.0f;
constexpr float kMaxLimiterReductionDb = 6.0f; // リミッターで潰してよい量の上限
constexpr float kGainMinDb = -20.0f;
constexpr float kGainMaxDb = 40.0f;
constexpr float kHighPassHz = 80.0f;

struct CompSettings {
	float threshold_db = -20.0f;
	float ratio = 4.0f;
};

// コンプの現在の状態（出力ゲインの計算に使う）
struct CompState {
	bool enabled = false;
	float threshold_db = -20.0f;
	float ratio = 3.0f;
};

CompSettings auto_comp(const Measurement &m);

// 出力ゲイン(dB)。コンプ後の声の平均を目標に合わせる。リミッターで潰れすぎない範囲に抑える。
float auto_gain_db(const Measurement &m, const CompState &comp, float limiter_ceiling_db);

float auto_limiter_ceiling_db();

float auto_highpass_hz();

// キーボード・マウスの音がはっきり測れたか（部屋のノイズより 6 dB 以上大きい）
bool clicks_present(const Measurement &m);

// キーボード・マウスの音が声に近すぎて、ゲートでは消しきれないか
bool clicks_near_voice(const Measurement &m);

// ノイズゲートのしきい値(dB)。部屋のノイズ + 10 dB。キーボード・マウスの音が測れていれば、それより +3 dB も超える
// （ただし声の平均 - 8 dB まで）。
float auto_gate_threshold_db(const Measurement &m);

// AI ノイズ抑制の強さ(%)。部屋のノイズが大きいほど強くする（50〜100）。
float auto_denoise_percent(const Measurement &m);

// EQ。声の帯域のバランスから補正を決め、選んだプリセットの味付けを足す。
vao::dsp::EqGains auto_eq(const Measurement &m, const vao::dsp::EqGains &preset);

// ディエッサーのしきい値(dB)。サ行の帯域の平均 + 1 dB（平均より強いサ行を抑える）。
float auto_deesser_threshold_db(const Measurement &m);

} // namespace vao::calib
