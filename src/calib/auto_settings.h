// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

// 測定結果から各フィルターの設定値を決める。OBS に依存しない純粋な計算（同じ入力なら同じ結果）。
// 数値は暫定。実機で調整する。

#pragma once

#include "dsp/eq.h"
#include "measurer.h"

namespace vao::calib {

constexpr float kTargetLufs = -18.0f; // 仕上がりの声の平均のラウドネス（LUFS）の標準。雑談配信向けの目安
constexpr float kTargetLufsMin =
	-18.0f; // 画面のスライダーの範囲: -18（雑談配信向け）〜 -14（ゲーム配信向け）、1 dB 刻み
constexpr float kTargetLufsMax = -14.0f;
constexpr float kLimiterCeilingDb = -3.0f;     // リミッターの基準の上限（dBFS）。Retzn さんの指定
constexpr float kMaxLimiterReductionDb = 6.0f; // リミッターで潰してよい量の上限
constexpr float kGainMinDb = -20.0f;
constexpr float kGainMaxDb = 40.0f;
constexpr float kHighPassHz = 80.0f;

// 入力ゲイン（チェーンの先頭）。声の平均をこの大きさにそろえてから、後ろの処理に渡す。
constexpr float kInputTargetLevelDb = -20.0f;
constexpr float kInputPeakCeilingDb = -1.0f; // 入力ゲイン後のピークがこれを超えないようにする
constexpr float kInputGainMinDb = -12.0f;
constexpr float kInputGainMaxDb = 40.0f;
constexpr float kQuietInputDb = -40.0f; // 声の平均がこれより小さければ、マイク側の音量を上げるよう案内する

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

// 出力ゲイン(dB)。コンプのあとの声の平均ラウドネスを target_lufs（既定 -18 LUFS）に合わせる。
// リミッターで潰れすぎない範囲に抑える。
// capped を渡すと、音割れ防止のため目標まで上げられなかったとき true になる。
float auto_gain_db(const Measurement &m, const CompState &comp, float limiter_ceiling_db,
		   float target_lufs = kTargetLufs, bool *capped = nullptr);

// 目標の音量(LUFS)を、スライダーの範囲に収める。数値でなければ標準（-18）。
float clamp_target_lufs(float target_lufs);

float auto_limiter_ceiling_db();

// 入力ゲイン(dB)。声の平均を kInputTargetLevelDb にそろえる。ピークが上限を超えない範囲に抑える。
float auto_input_gain_db(const Measurement &m);

// 測定値のうち「大きさ」を表す値（ノイズ・声・ピーク・帯域など）を、gain_db だけずらす。
// 入力ゲインを通ったあとの大きさで、後ろの処理の自動設定を計算するために使う。
Measurement shift_levels(const Measurement &m, float gain_db);

// 声がとても小さい（マイク側の音量を先に上げたほうがよい）か
bool input_too_quiet(const Measurement &m);

float auto_highpass_hz();

// キーボード・マウスの音がはっきり測れたか（部屋のノイズより 6 dB 以上大きい）
bool clicks_present(const Measurement &m);

// キーボード・マウスの音が声に近すぎて、ゲートでは消しきれないか
bool clicks_near_voice(const Measurement &m);

// ノイズゲートのしきい値(dB)。部屋のノイズ + 10 dB。キーボード・マウスの音が測れていれば、それより +3 dB も超える
// （ただし声の平均 - 8 dB まで）。
float auto_gate_threshold_db(const Measurement &m);

// AI ノイズ抑制の強さ(%)。声に対して雑音が大きいほど強くする（50〜100）。
float auto_denoise_percent(const Measurement &m);

// EQ。声の帯域のバランスから補正を決め、選んだプリセットの味付けを足す。
vao::dsp::EqGains auto_eq(const Measurement &m, const vao::dsp::EqGains &preset);

// ディエッサーのしきい値(dB)。サ行の帯域の平均 + 1 dB（平均より強いサ行を抑える）。
float auto_deesser_threshold_db(const Measurement &m);

} // namespace vao::calib
