// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

#include "gain.h"

#include <cmath>

namespace vao::dsp {

float db_to_linear(float db)
{
	if (!(db > kMinGainDb)) // NaN もここに入る
		return 0.0f;
	if (db > kMaxGainDb)
		db = kMaxGainDb;
	return std::pow(10.0f, db / 20.0f);
}

void Gain::set_gain_db(float db)
{
	if (std::isnan(db))
		return;
	db_ = db < kMinGainDb ? kMinGainDb : (db > kMaxGainDb ? kMaxGainDb : db);
	target_ = db_to_linear(db_);
}

void Gain::reset()
{
	current_ = target_;
}

void Gain::process(float *const *channels, std::size_t num_channels, std::size_t num_frames)
{
	if (channels == nullptr || num_frames == 0)
		return;

	if (current_ == target_) {
		if (target_ == 1.0f)
			return;
		for (std::size_t c = 0; c < num_channels; ++c) {
			float *buf = channels[c];
			if (buf == nullptr)
				continue;
			for (std::size_t i = 0; i < num_frames; ++i)
				buf[i] *= target_;
		}
		return;
	}

	// ブロックの最後でちょうど目標に着く直線の変化
	const float start = current_;
	const float step = (target_ - start) / static_cast<float>(num_frames);
	for (std::size_t c = 0; c < num_channels; ++c) {
		float *buf = channels[c];
		if (buf == nullptr)
			continue;
		for (std::size_t i = 0; i < num_frames; ++i)
			buf[i] *= start + step * static_cast<float>(i + 1);
	}
	current_ = target_;
}

} // namespace vao::dsp
