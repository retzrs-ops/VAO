// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

#include "limiter.h"

#include <algorithm>
#include <cmath>

namespace vao::dsp {

// 仕組み: 各フレームで「上限に収めるのに必要な倍率」g を求め、
// 直近 L フレームの最小値 → さらに L フレームの平均、で滑らかな倍率にする。
// 音は L-1 フレーム遅らせて出すので、ピークが来る時点で倍率が g 以下になっている。

void Limiter::prepare(float sample_rate)
{
	sample_rate_ = sample_rate > 0.0f ? sample_rate : 48000.0f;
	lookahead_ = std::max<std::size_t>(2, static_cast<std::size_t>(std::ceil(sample_rate_ * 0.002f)));
	delay_.assign(kMaxChannels * lookahead_, 0.0f);
	need_.assign(lookahead_, 1.0f);
	minw_.assign(lookahead_, 1.0f);
	release_k_ = 1.0f - std::exp(-1.0f / (0.1f * sample_rate_)); // 戻りは約 100 ms
	set_params(enabled_, ceiling_db_);
	reset();
}

void Limiter::set_params(bool enabled, float ceiling_db)
{
	enabled_ = enabled;
	if (!std::isnan(ceiling_db))
		ceiling_db_ = std::clamp(ceiling_db, -24.0f, 0.0f);
	ceiling_ = std::pow(10.0f, ceiling_db_ / 20.0f);
}

void Limiter::reset()
{
	std::fill(delay_.begin(), delay_.end(), 0.0f);
	std::fill(need_.begin(), need_.end(), 1.0f);
	std::fill(minw_.begin(), minw_.end(), 1.0f);
	pos_ = 0;
	gain_prev_ = 1.0f;
}

void Limiter::process(float *const *channels, std::size_t num_channels, std::size_t num_frames)
{
	if (channels == nullptr || num_frames == 0 || lookahead_ == 0 || num_channels == 0 ||
	    num_channels > kMaxChannels)
		return;

	const std::size_t L = lookahead_;
	const float inv_L = 1.0f / static_cast<float>(L);

	for (std::size_t i = 0; i < num_frames; ++i) {
		float peak = 0.0f;
		for (std::size_t c = 0; c < num_channels; ++c) {
			if (channels[c] != nullptr)
				peak = std::max(peak, std::fabs(channels[c][i]));
		}

		float need = 1.0f;
		if (enabled_ && peak > ceiling_)
			need = ceiling_ / peak;
		need_[pos_] = need;

		float window_min = 1.0f;
		for (std::size_t k = 0; k < L; ++k)
			window_min = std::min(window_min, need_[k]);
		minw_[pos_] = window_min;

		float sum = 0.0f;
		for (std::size_t k = 0; k < L; ++k)
			sum += minw_[k];
		const float avg = sum * inv_L;

		// 上がる向きだけゆっくり（下がる向きは avg にそのまま従う）
		float g = avg;
		if (avg > gain_prev_)
			g = std::min(avg, gain_prev_ + (1.0f - gain_prev_) * release_k_);
		gain_prev_ = g;

		const std::size_t read = (pos_ + 1) % L; // L-1 フレーム前に書いた位置
		for (std::size_t c = 0; c < num_channels; ++c) {
			float *slot = &delay_[c * L];
			slot[pos_] = channels[c] != nullptr ? channels[c][i] : 0.0f;
			if (channels[c] == nullptr)
				continue;
			float y = slot[read] * g;
			if (enabled_)
				y = std::clamp(y, -ceiling_, ceiling_); // 念のための最終ガード
			channels[c][i] = y;
		}
		pos_ = read;
	}
}

} // namespace vao::dsp
