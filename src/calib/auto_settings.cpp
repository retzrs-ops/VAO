// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

#include "auto_settings.h"

#include <algorithm>

namespace vao::calib {

CompSettings auto_comp(const Measurement &m)
{
	CompSettings c;
	c.ratio = 4.0f;
	c.threshold_db = std::clamp(m.speech_level_db - 6.0f, -60.0f, -3.0f);
	return c;
}

float auto_gain_db(const Measurement &m, const CompState &comp, float limiter_ceiling_db)
{
	float avg_reduction = 0.0f;
	float peak_after = m.peak_db;
	if (comp.enabled && comp.ratio > 1.0f) {
		const float slope = 1.0f - 1.0f / comp.ratio;
		avg_reduction = std::max(0.0f, m.speech_level_db - comp.threshold_db) * slope;
		if (m.peak_db > comp.threshold_db)
			peak_after = comp.threshold_db + (m.peak_db - comp.threshold_db) / comp.ratio;
	}

	float gain = kTargetSpeechLevelDb - (m.speech_level_db - avg_reduction);
	// 大声のピークがリミッターで潰れすぎない範囲に抑える
	gain = std::min(gain, limiter_ceiling_db + kMaxLimiterReductionDb - peak_after);
	return std::clamp(gain, kGainMinDb, kGainMaxDb);
}

float auto_limiter_ceiling_db()
{
	return kLimiterCeilingDb;
}

float auto_highpass_hz()
{
	return kHighPassHz;
}

namespace {

float loudest_click_db(const Measurement &m)
{
	return std::max(m.keyboard_level_db, m.mouse_level_db);
}

} // namespace

bool clicks_present(const Measurement &m)
{
	return loudest_click_db(m) > m.noise_floor_db + 6.0f;
}

bool clicks_near_voice(const Measurement &m)
{
	return clicks_present(m) && loudest_click_db(m) + 3.0f > m.speech_level_db - 8.0f;
}

float auto_gate_threshold_db(const Measurement &m)
{
	float thr = std::clamp(m.noise_floor_db + 10.0f, -80.0f, -25.0f);
	// 声の平均より 12 dB 以上は下に置く（声を切らないため）
	thr = std::min(thr, m.speech_level_db - 12.0f);
	if (clicks_present(m)) {
		// キーボード・マウスの音より上に置く。ただし声の平均 - 8 dB まで
		const float click_thr = std::min(loudest_click_db(m) + 3.0f, m.speech_level_db - 8.0f);
		thr = std::max(thr, click_thr);
	}
	return std::clamp(thr, -90.0f, -25.0f);
}

float auto_denoise_percent(const Measurement &m)
{
	// ノイズ -75 dB で 50%、-50 dB 以上で 100%。キーボード・マウスの音が出る環境では 100%
	if (clicks_present(m))
		return 100.0f;
	return std::clamp(50.0f + (m.noise_floor_db + 75.0f) * 2.0f, 50.0f, 100.0f);
}

vao::dsp::EqGains auto_eq(const Measurement &m, const vao::dsp::EqGains &preset)
{
	// こもり(300 Hz)が明瞭さ(3 kHz)より強いほどこもっている（逆なら高域が強い）
	const float mud_vs_pres = m.band_mud_db - m.band_pres_db;
	const float low_vs_mud = m.band_low_db - m.band_mud_db;

	vao::dsp::EqGains g;
	g.mud_db = -std::clamp(mud_vs_pres * 0.7f, 0.0f, 6.0f);
	if (mud_vs_pres > 2.0f)
		g.pres_db =
			std::clamp((mud_vs_pres - 2.0f) * 0.7f, 0.0f, 6.0f); // こもっていて抜けが悪い → 明瞭さを足す
	else if (mud_vs_pres < -8.0f)
		g.pres_db = -std::clamp((-mud_vs_pres - 8.0f) * 0.7f, 0.0f, 4.0f); // 高域が強すぎる → 少し抑える
	g.low_db = -std::clamp((low_vs_mud - 2.0f) * 0.7f, 0.0f, 6.0f);            // 近づきすぎの低域のふくらみ
	g.air_db = 0.0f;

	const float lim = vao::dsp::kEqMaxDb;
	g.low_db = std::clamp(g.low_db + preset.low_db, -lim, lim);
	g.mud_db = std::clamp(g.mud_db + preset.mud_db, -lim, lim);
	g.pres_db = std::clamp(g.pres_db + preset.pres_db, -lim, lim);
	g.air_db = std::clamp(g.air_db + preset.air_db, -lim, lim);
	return g;
}

float auto_deesser_threshold_db(const Measurement &m)
{
	return std::clamp(m.band_sib_db + 1.0f, -70.0f, -10.0f);
}

} // namespace vao::calib
