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

} // namespace vao::calib
