// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

// 測定結果から各フィルターの設定値を決める。OBS に依存しない純粋な計算（同じ入力なら同じ結果）。
// 数値は暫定。実機で調整する。

#pragma once

#include "measurer.h"

namespace vao::calib {

constexpr float kTargetSpeechLevelDb = -18.0f; // 声の平均の目標
constexpr float kLimiterCeilingDb = -1.0f;
constexpr float kMaxLimiterReductionDb = 6.0f; // リミッターで潰してよい量の上限
constexpr float kGainMinDb = -20.0f;
constexpr float kGainMaxDb = 40.0f;

struct CompSettings {
	float threshold_db = -20.0f;
	float ratio = 3.0f;
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

} // namespace vao::calib
