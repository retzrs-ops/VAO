// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

#include "denoiser.h"

#include <algorithm>
#include <cmath>

#include <rnnoise.h>

namespace vao::dsp {

namespace {

// RNNoise は 16 ビット整数の大きさの float を受け取る
constexpr float kScale = 32768.0f;

} // namespace

Denoiser::Denoiser() = default;

Denoiser::~Denoiser()
{
	for (auto &c : ch_) {
		if (c.state != nullptr)
			rnnoise_destroy(c.state);
	}
}

bool Denoiser::prepare(float sample_rate)
{
	supported_ = std::lround(sample_rate) == 48000;
	if (!supported_)
		return false;
	for (auto &c : ch_) {
		if (c.state == nullptr)
			c.state = rnnoise_create(nullptr); // 内蔵モデル
		if (c.state == nullptr) {
			supported_ = false;
			return false;
		}
	}
	reset();
	return true;
}

void Denoiser::set_params(bool enabled, float strength)
{
	enabled_ = enabled;
	if (!std::isnan(strength))
		strength_ = std::clamp(strength, 0.0f, 1.0f);
}

void Denoiser::reset_channel(Channel &c)
{
	std::fill(std::begin(c.in), std::end(c.in), 0.0f);
	std::fill(std::begin(c.out), std::end(c.out), 0.0f);
	c.in_fill = 0;
	c.out_read = 0;
	c.out_count = kFrame; // 最初は無音を kFrame 分出す（これが固定の遅れ）
}

void Denoiser::reset()
{
	for (auto &c : ch_)
		reset_channel(c);
}

void Denoiser::run_frame(Channel &c)
{
	float result[kFrame];
	if (enabled_ && strength_ > 0.0f && c.state != nullptr) {
		float scaled[kFrame];
		float wet[kFrame];
		for (std::size_t k = 0; k < kFrame; ++k)
			scaled[k] = c.in[k] * kScale;
		rnnoise_process_frame(c.state, wet, scaled);
		const float s = strength_;
		for (std::size_t k = 0; k < kFrame; ++k)
			result[k] = c.in[k] * (1.0f - s) + (wet[k] / kScale) * s;
	} else {
		for (std::size_t k = 0; k < kFrame; ++k)
			result[k] = c.in[k];
	}

	std::size_t write = (c.out_read + c.out_count) % kRing;
	for (std::size_t k = 0; k < kFrame; ++k) {
		c.out[write] = result[k];
		write = (write + 1) % kRing;
	}
	c.out_count += kFrame;
}

void Denoiser::process(float *const *channels, std::size_t num_channels, std::size_t num_frames)
{
	if (!supported_ || channels == nullptr || num_frames == 0 || num_channels == 0 || num_channels > kMaxChannels)
		return;

	for (std::size_t ci = 0; ci < num_channels; ++ci) {
		float *buf = channels[ci];
		if (buf == nullptr)
			continue;
		Channel &c = ch_[ci];
		for (std::size_t i = 0; i < num_frames; ++i) {
			c.in[c.in_fill++] = buf[i];
			if (c.in_fill == kFrame) {
				run_frame(c);
				c.in_fill = 0;
			}
			buf[i] = c.out[c.out_read];
			c.out_read = (c.out_read + 1) % kRing;
			--c.out_count;
		}
	}
}

} // namespace vao::dsp
