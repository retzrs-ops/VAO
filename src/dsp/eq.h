// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

// 4 バンドの EQ と、味付けのプリセット。オフ（または全部 0 dB）のときは音を変えない。
//   低域 100 Hz のシェルフ / こもり 300 Hz / 明瞭さ 3 kHz / 空気感 8 kHz のシェルフ

#pragma once

#include <cstddef>

#include "biquad.h"

namespace vao::dsp {

struct EqGains {
	float low_db = 0.0f;  // 低域（100 Hz 以下のシェルフ）
	float mud_db = 0.0f;  // こもり（300 Hz 付近）
	float pres_db = 0.0f; // 明瞭さ（3 kHz 付近）
	float air_db = 0.0f;  // 空気感（8 kHz 以上のシェルフ）
};

constexpr float kEqMaxDb = 12.0f;

constexpr int kEqPresetCount = 6;
// 0:ナチュラル 1:クリア 2:あたたかい 3:ラジオ 4:やわらか 5:ハキハキ。範囲外は 0（ナチュラル）。数値は暫定。
EqGains eq_preset(int index);

class Eq {
public:
	void prepare(float sample_rate); // 音声スレッドでは呼ばない
	void set_params(bool enabled, const EqGains &gains);
	void reset();
	void process(float *const *channels, std::size_t num_channels, std::size_t num_frames);

private:
	static constexpr int kBands = 4;
	double sample_rate_ = 48000.0;
	bool enabled_ = false;
	bool flat_ = true;
	EqGains applied_{};
	bool has_applied_ = false;
	BiquadBank bank_[kBands];
};

} // namespace vao::dsp
