// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

#include "highpass.h"

#include <algorithm>
#include <cmath>

namespace vao::dsp {

void HighPass::prepare(float sample_rate)
{
	sample_rate_ = sample_rate > 0.0f ? sample_rate : 48000.0;
	freq_hz_ = 0.0f;
	reset();
}

void HighPass::set_params(bool enabled, float freq_hz)
{
	if (!enabled_ && enabled)
		reset(); // オンにする瞬間に古い状態を持ち込まない
	enabled_ = enabled;
	if (std::isnan(freq_hz))
		return;
	freq_hz = std::clamp(freq_hz, 20.0f, 400.0f);
	if (freq_hz == freq_hz_)
		return;
	freq_hz_ = freq_hz;
	// 4 次バターワースの2段分の Q
	const BiquadCoefs c1 = biquad_highpass(sample_rate_, freq_hz, 0.54119610);
	const BiquadCoefs c2 = biquad_highpass(sample_rate_, freq_hz, 1.30656296);
	for (auto &b : bank_[0])
		b.set_coefs(c1);
	for (auto &b : bank_[1])
		b.set_coefs(c2);
}

void HighPass::reset()
{
	for (auto &bank : bank_)
		for (auto &b : bank)
			b.reset();
}

void HighPass::process(float *const *channels, std::size_t num_channels, std::size_t num_frames)
{
	if (!enabled_ || channels == nullptr || num_frames == 0 || num_channels > kBiquadMaxChannels)
		return;
	for (std::size_t c = 0; c < num_channels; ++c) {
		float *buf = channels[c];
		if (buf == nullptr)
			continue;
		for (std::size_t i = 0; i < num_frames; ++i)
			buf[i] = bank_[1][c].process(bank_[0][c].process(buf[i]));
	}
}

} // namespace vao::dsp
