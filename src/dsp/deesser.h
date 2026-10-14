// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

// ディエッサー（「さしすせそ」の刺さりを抑える）。サ行の帯域だけを下げる。全チャンネル連動。
// 検出は「サ行の帯域の二乗平均(dB)」。しきい値は calib の測定値（同じ帯域・同じ単位）に合わせる。

#pragma once

#include <cstddef>

#include "biquad.h"

namespace vao::dsp {

constexpr double kDeesserFreqHz = 7000.0;
constexpr double kDeesserQ = 1.2;

class Deesser {
public:
	static constexpr float kRatio = 6.0f;
	static constexpr float kMaxReductionDb = 15.0f;

	void prepare(float sample_rate); // 音声スレッドでは呼ばない
	void set_params(bool enabled, float threshold_db);
	void reset();
	void process(float *const *channels, std::size_t num_channels, std::size_t num_frames);

private:
	float sample_rate_ = 48000.0f;
	bool enabled_ = false;
	float threshold_db_ = -40.0f;
	float attack_k_ = 0.0f;
	float release_k_ = 0.0f;
	float power_ = 0.0f;
	BiquadBank bank_;
};

} // namespace vao::dsp
