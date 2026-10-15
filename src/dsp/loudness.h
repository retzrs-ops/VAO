// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

// ラウドネス（LUFS）の測定に使う K 特性フィルタ（ITU-R BS.1770）。OBS・Qt に依存しない。
// 音を「人の耳で感じる大きさ」に近づける重み付け: ① 高域を約 +4 dB 持ち上げ → ② 低い音（約 38 Hz 以下）を切る。
// LUFS = -0.691 + 10 * log10( チャンネルごとの「K 特性をかけた音の二乗平均」の合計 )

#pragma once

#include "biquad.h"

namespace vao::dsp {

constexpr float kLufsOffsetDb = -0.691f;

// ① 高域を持ち上げるシェルフ
BiquadCoefs kweight_prefilter(double sample_rate);
// ② 低域を切るハイパス（RLB）
BiquadCoefs kweight_rlb(double sample_rate);

} // namespace vao::dsp
