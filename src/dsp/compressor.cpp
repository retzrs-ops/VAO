// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

#include "compressor.h"

#include <algorithm>
#include <cmath>

namespace vao::dsp {

namespace {

float time_coef(float ms, float sample_rate)
{
	const float samples = std::max(ms, 0.1f) * 0.001f * sample_rate;
	return std::exp(-1.0f / samples);
}

} // namespace

void Compressor::prepare(float sample_rate)
{
	sample_rate_ = sample_rate > 0.0f ? sample_rate : 48000.0f;
	set_params(enabled_, threshold_db_, ratio_);
	reset();
}

void Compressor::set_params(bool enabled, float threshold_db, float ratio, float attack_ms, float release_ms)
{
	enabled_ = enabled;
	if (!std::isnan(threshold_db))
		threshold_db_ = std::clamp(threshold_db, -80.0f, 0.0f);
	if (!std::isnan(ratio))
		ratio_ = std::clamp(ratio, 1.0f, 20.0f);
	attack_coef_ = time_coef(attack_ms, sample_rate_);
	release_coef_ = time_coef(release_ms, sample_rate_);
}

void Compressor::reset()
{
	env_ = 0.0f;
	gr_db_ = 0.0f;
}

void Compressor::process(float *const *channels, std::size_t num_channels, std::size_t num_frames)
{
	if (channels == nullptr || num_frames == 0)
		return;
	if (!enabled_ && gr_db_ < 0.001f) {
		gr_db_ = 0.0f;
		env_ = 0.0f;
		return;
	}

	const float slope = 1.0f - 1.0f / ratio_;
	for (std::size_t i = 0; i < num_frames; ++i) {
		float peak = 0.0f;
		for (std::size_t c = 0; c < num_channels; ++c) {
			if (channels[c] != nullptr)
				peak = std::max(peak, std::fabs(channels[c][i]));
		}

		const float coef = peak > env_ ? attack_coef_ : release_coef_;
		env_ = coef * env_ + (1.0f - coef) * peak;

		if (enabled_) {
			const float level_db = 20.0f * std::log10(std::max(env_, 1e-6f));
			const float over = level_db - threshold_db_;
			gr_db_ = over > 0.0f ? over * slope : 0.0f;
		} else {
			gr_db_ *= release_coef_; // オフにした直後はゆっくり元に戻す
		}

		const float g = std::pow(10.0f, -gr_db_ / 20.0f);
		for (std::size_t c = 0; c < num_channels; ++c) {
			if (channels[c] != nullptr)
				channels[c][i] *= g;
		}
	}
}

} // namespace vao::dsp
