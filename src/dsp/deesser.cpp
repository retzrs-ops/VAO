// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

#include "deesser.h"

#include <algorithm>
#include <cmath>

namespace vao::dsp {

namespace {

float time_k(float ms, float sample_rate)
{
	return 1.0f - std::exp(-1.0f / (ms * 0.001f * sample_rate));
}

} // namespace

void Deesser::prepare(float sample_rate)
{
	sample_rate_ = sample_rate > 0.0f ? sample_rate : 48000.0f;
	attack_k_ = time_k(1.0f, sample_rate_);
	release_k_ = time_k(40.0f, sample_rate_);
	const BiquadCoefs c = biquad_bandpass(sample_rate_, kDeesserFreqHz, kDeesserQ);
	for (auto &b : bank_)
		b.set_coefs(c);
	reset();
}

void Deesser::set_params(bool enabled, float threshold_db)
{
	if (!enabled_ && enabled)
		reset();
	enabled_ = enabled;
	if (!std::isnan(threshold_db))
		threshold_db_ = std::clamp(threshold_db, -90.0f, -5.0f);
}

void Deesser::reset()
{
	power_ = 0.0f;
	for (auto &b : bank_)
		b.reset();
}

void Deesser::process(float *const *channels, std::size_t num_channels, std::size_t num_frames)
{
	if (!enabled_ || channels == nullptr || num_frames == 0 || num_channels > kBiquadMaxChannels)
		return;

	const float slope = 1.0f - 1.0f / kRatio;
	float band[kBiquadMaxChannels];
	for (std::size_t i = 0; i < num_frames; ++i) {
		float p = 0.0f;
		for (std::size_t c = 0; c < num_channels; ++c) {
			band[c] = channels[c] != nullptr ? bank_[c].process(channels[c][i]) : 0.0f;
			p = std::max(p, band[c] * band[c]);
		}
		power_ += (p - power_) * (p > power_ ? attack_k_ : release_k_);

		const float level_db = 10.0f * std::log10(power_ + 1e-12f);
		const float over = level_db - threshold_db_;
		const float reduction_db = over > 0.0f ? std::min(over * slope, kMaxReductionDb) : 0.0f;
		const float g = std::pow(10.0f, -reduction_db / 20.0f);

		// サ行の帯域だけを g 倍にする（それ以外はそのまま）
		for (std::size_t c = 0; c < num_channels; ++c) {
			if (channels[c] != nullptr)
				channels[c][i] += (g - 1.0f) * band[c];
		}
	}
}

} // namespace vao::dsp
