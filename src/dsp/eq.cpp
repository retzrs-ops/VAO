// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

#include "eq.h"

#include <algorithm>
#include <cmath>

namespace vao::dsp {

EqGains eq_preset(int index)
{
	switch (index) {
	case 1: // クリア: こもりを抜き、明瞭さと空気感を足す
		return {-3.0f, -4.0f, 5.0f, 3.0f};
	case 2: // あたたかい: 低域を厚く、高域は控えめ
		return {5.0f, 2.0f, -2.0f, -3.0f};
	case 3: // ラジオ: 太く、前に出る
		return {6.0f, 1.0f, 3.0f, -3.0f};
	case 4: // やわらか: 角を落とす
		return {2.0f, -2.0f, -3.0f, -5.0f};
	case 5: // ハキハキ: 元気で抜けがよい
		return {-2.0f, -3.0f, 6.0f, 4.0f};
	default: // ナチュラル
		return {};
	}
}

namespace {

float limit(float db)
{
	return std::isnan(db) ? 0.0f : std::clamp(db, -kEqMaxDb, kEqMaxDb);
}

bool same(const EqGains &a, const EqGains &b)
{
	return a.low_db == b.low_db && a.mud_db == b.mud_db && a.pres_db == b.pres_db && a.air_db == b.air_db;
}

} // namespace

void Eq::prepare(float sample_rate)
{
	sample_rate_ = sample_rate > 0.0f ? sample_rate : 48000.0;
	has_applied_ = false;
	reset();
}

void Eq::set_params(bool enabled, const EqGains &gains)
{
	if (!enabled_ && enabled)
		reset();
	enabled_ = enabled;

	EqGains g;
	g.low_db = limit(gains.low_db);
	g.mud_db = limit(gains.mud_db);
	g.pres_db = limit(gains.pres_db);
	g.air_db = limit(gains.air_db);
	if (has_applied_ && same(g, applied_))
		return;
	applied_ = g;
	has_applied_ = true;

	flat_ = std::fabs(g.low_db) < 0.05f && std::fabs(g.mud_db) < 0.05f && std::fabs(g.pres_db) < 0.05f &&
		std::fabs(g.air_db) < 0.05f;

	const BiquadCoefs coefs[kBands] = {
		biquad_lowshelf(sample_rate_, 100.0, g.low_db),
		biquad_peaking(sample_rate_, 300.0, 1.0, g.mud_db),
		biquad_peaking(sample_rate_, 3000.0, 1.0, g.pres_db),
		biquad_highshelf(sample_rate_, 8000.0, g.air_db),
	};
	for (int b = 0; b < kBands; ++b)
		for (auto &bq : bank_[b])
			bq.set_coefs(coefs[b]);
}

void Eq::reset()
{
	for (auto &bank : bank_)
		for (auto &bq : bank)
			bq.reset();
}

void Eq::process(float *const *channels, std::size_t num_channels, std::size_t num_frames)
{
	if (!enabled_ || flat_ || channels == nullptr || num_frames == 0 || num_channels > kBiquadMaxChannels)
		return;
	for (std::size_t c = 0; c < num_channels; ++c) {
		float *buf = channels[c];
		if (buf == nullptr)
			continue;
		for (std::size_t i = 0; i < num_frames; ++i) {
			float x = buf[i];
			for (int b = 0; b < kBands; ++b)
				x = bank_[b][c].process(x);
			buf[i] = x;
		}
	}
}

} // namespace vao::dsp
