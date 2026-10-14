// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

// ハイパスフィルタ（低い雑音のカット）。4 次（2 次を 2 段）、24 dB/oct。オフのときは音を変えない。

#pragma once

#include <cstddef>

#include "biquad.h"

namespace vao::dsp {

class HighPass {
public:
	void prepare(float sample_rate); // 音声スレッドでは呼ばない
	void set_params(bool enabled, float freq_hz);
	void reset();
	void process(float *const *channels, std::size_t num_channels, std::size_t num_frames);

private:
	double sample_rate_ = 48000.0;
	bool enabled_ = false;
	float freq_hz_ = 0.0f; // 係数を作った周波数
	BiquadBank bank_[2];
};

} // namespace vao::dsp
