// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

#include "loudness.h"

#include <cmath>

namespace vao::dsp {

namespace {
constexpr double kPi = 3.14159265358979323846;
}

// ITU-R BS.1770 のアナログ原型を、双一次変換で任意のサンプルレートに直したもの。
// 48 kHz のとき、規格書の係数（b0=1.53512485958697 など）と一致する。

BiquadCoefs kweight_prefilter(double fs)
{
	const double f0 = 1681.974450955533;
	const double G = 3.999843853973347;
	const double Q = 0.7071752369554196;

	const double K = std::tan(kPi * f0 / fs);
	const double Vh = std::pow(10.0, G / 20.0);
	const double Vb = std::pow(Vh, 0.4996667741545416);
	const double a0 = 1.0 + K / Q + K * K;

	BiquadCoefs c;
	c.b0 = (Vh + Vb * K / Q + K * K) / a0;
	c.b1 = 2.0 * (K * K - Vh) / a0;
	c.b2 = (Vh - Vb * K / Q + K * K) / a0;
	c.a1 = 2.0 * (K * K - 1.0) / a0;
	c.a2 = (1.0 - K / Q + K * K) / a0;
	return c;
}

BiquadCoefs kweight_rlb(double fs)
{
	const double f0 = 38.13547087602444;
	const double Q = 0.5003270373238773;

	const double K = std::tan(kPi * f0 / fs);
	const double a0 = 1.0 + K / Q + K * K;

	BiquadCoefs c;
	c.b0 = 1.0; // 規格どおり、分子は 1, -2, 1（a0 で割らない）
	c.b1 = -2.0;
	c.b2 = 1.0;
	c.a1 = 2.0 * (K * K - 1.0) / a0;
	c.a2 = (1.0 - K / Q + K * K) / a0;
	return c;
}

} // namespace vao::dsp
