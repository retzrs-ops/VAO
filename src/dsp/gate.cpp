// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

#include "gate.h"

#include <algorithm>
#include <cmath>

namespace vao::dsp {

namespace {

constexpr float kHysteresisDb = 3.0f;

float time_k(float ms, float sample_rate)
{
	return 1.0f - std::exp(-1.0f / (ms * 0.001f * sample_rate));
}

} // namespace

void NoiseGate::prepare(float sample_rate)
{
	sample_rate_ = sample_rate > 0.0f ? sample_rate : 48000.0f;
	level_k_ = time_k(10.0f, sample_rate_);
	attack_k_ = time_k(3.0f, sample_rate_);
	release_k_ = time_k(200.0f, sample_rate_);
	hold_samples_ = static_cast<std::size_t>(0.15f * sample_rate_);
	reset();
}

void NoiseGate::set_params(bool enabled, float threshold_db)
{
	enabled_ = enabled;
	if (!std::isnan(threshold_db))
		threshold_db_ = std::clamp(threshold_db, -90.0f, -10.0f);
}

void NoiseGate::reset()
{
	power_ = 0.0f;
	open_ = true; // 最初の音を欠かさない
	hold_ = hold_samples_;
	gain_ = 1.0f;
}

void NoiseGate::process(float *const *channels, std::size_t num_channels, std::size_t num_frames)
{
	if (channels == nullptr || num_frames == 0)
		return;
	if (!enabled_ && gain_ >= 0.9999f) {
		gain_ = 1.0f;
		return;
	}

	const float closed_gain = std::pow(10.0f, kClosedGainDb / 20.0f);
	for (std::size_t i = 0; i < num_frames; ++i) {
		float p = 0.0f;
		for (std::size_t c = 0; c < num_channels; ++c) {
			if (channels[c] != nullptr)
				p = std::max(p, channels[c][i] * channels[c][i]);
		}
		power_ += (p - power_) * level_k_;
		const float level_db = 10.0f * std::log10(power_ + 1e-12f);

		if (level_db > threshold_db_) {
			open_ = true;
			hold_ = hold_samples_;
		} else if (level_db < threshold_db_ - kHysteresisDb) {
			if (hold_ > 0)
				--hold_;
			else
				open_ = false;
		}

		const float target = (!enabled_ || open_) ? 1.0f : closed_gain;
		gain_ += (target - gain_) * (target > gain_ ? attack_k_ : release_k_);

		for (std::size_t c = 0; c < num_channels; ++c) {
			if (channels[c] != nullptr)
				channels[c][i] *= gain_;
		}
	}
}

} // namespace vao::dsp
