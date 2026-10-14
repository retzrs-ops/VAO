// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

// 2 次フィルタ（biquad）。係数は RBJ の式。OBS・Qt に依存しない。
// process() は確保・ロック・ログをしない。

#pragma once

#include <array>
#include <cstddef>

namespace vao::dsp {

constexpr std::size_t kBiquadMaxChannels = 8;

struct BiquadCoefs {
	double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0; // a0 で割り済み
};

BiquadCoefs biquad_highpass(double sample_rate, double freq_hz, double q);
// ピークの利得が 0 dB のバンドパス
BiquadCoefs biquad_bandpass(double sample_rate, double freq_hz, double q);
BiquadCoefs biquad_peaking(double sample_rate, double freq_hz, double q, double gain_db);
BiquadCoefs biquad_lowshelf(double sample_rate, double freq_hz, double gain_db);
BiquadCoefs biquad_highshelf(double sample_rate, double freq_hz, double gain_db);

// 1 チャンネル分の状態つきフィルタ
class Biquad {
public:
	void set_coefs(const BiquadCoefs &c) { c_ = c; }
	void reset() { z1_ = z2_ = 0.0; }
	float process(float x)
	{
		const double in = x;
		const double out = c_.b0 * in + z1_;
		z1_ = c_.b1 * in - c_.a1 * out + z2_;
		z2_ = c_.b2 * in - c_.a2 * out;
		return static_cast<float>(out);
	}

private:
	BiquadCoefs c_;
	double z1_ = 0.0;
	double z2_ = 0.0;
};

using BiquadBank = std::array<Biquad, kBiquadMaxChannels>;

} // namespace vao::dsp
