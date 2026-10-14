// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

#include "biquad.h"

#include <algorithm>
#include <cmath>

namespace vao::dsp {

namespace {

constexpr double kPi = 3.14159265358979323846;

BiquadCoefs normalized(double b0, double b1, double b2, double a0, double a1, double a2)
{
	BiquadCoefs c;
	c.b0 = b0 / a0;
	c.b1 = b1 / a0;
	c.b2 = b2 / a0;
	c.a1 = a1 / a0;
	c.a2 = a2 / a0;
	return c;
}

// 周波数は 10 Hz〜ナイキスト手前に丸める
double clamp_freq(double sample_rate, double freq_hz)
{
	return std::clamp(freq_hz, 10.0, sample_rate * 0.45);
}

} // namespace

BiquadCoefs biquad_highpass(double fs, double f, double q)
{
	const double w0 = 2.0 * kPi * clamp_freq(fs, f) / fs;
	const double cs = std::cos(w0);
	const double alpha = std::sin(w0) / (2.0 * q);
	return normalized((1.0 + cs) / 2.0, -(1.0 + cs), (1.0 + cs) / 2.0, 1.0 + alpha, -2.0 * cs, 1.0 - alpha);
}

BiquadCoefs biquad_bandpass(double fs, double f, double q)
{
	const double w0 = 2.0 * kPi * clamp_freq(fs, f) / fs;
	const double cs = std::cos(w0);
	const double alpha = std::sin(w0) / (2.0 * q);
	return normalized(alpha, 0.0, -alpha, 1.0 + alpha, -2.0 * cs, 1.0 - alpha);
}

BiquadCoefs biquad_peaking(double fs, double f, double q, double gain_db)
{
	const double A = std::pow(10.0, gain_db / 40.0);
	const double w0 = 2.0 * kPi * clamp_freq(fs, f) / fs;
	const double cs = std::cos(w0);
	const double alpha = std::sin(w0) / (2.0 * q);
	return normalized(1.0 + alpha * A, -2.0 * cs, 1.0 - alpha * A, 1.0 + alpha / A, -2.0 * cs, 1.0 - alpha / A);
}

BiquadCoefs biquad_lowshelf(double fs, double f, double gain_db)
{
	const double A = std::pow(10.0, gain_db / 40.0);
	const double w0 = 2.0 * kPi * clamp_freq(fs, f) / fs;
	const double cs = std::cos(w0);
	const double alpha = std::sin(w0) / std::sqrt(2.0); // 傾き S = 1
	const double beta = 2.0 * std::sqrt(A) * alpha;
	return normalized(A * ((A + 1.0) - (A - 1.0) * cs + beta), 2.0 * A * ((A - 1.0) - (A + 1.0) * cs),
			  A * ((A + 1.0) - (A - 1.0) * cs - beta), (A + 1.0) + (A - 1.0) * cs + beta,
			  -2.0 * ((A - 1.0) + (A + 1.0) * cs), (A + 1.0) + (A - 1.0) * cs - beta);
}

BiquadCoefs biquad_highshelf(double fs, double f, double gain_db)
{
	const double A = std::pow(10.0, gain_db / 40.0);
	const double w0 = 2.0 * kPi * clamp_freq(fs, f) / fs;
	const double cs = std::cos(w0);
	const double alpha = std::sin(w0) / std::sqrt(2.0);
	const double beta = 2.0 * std::sqrt(A) * alpha;
	return normalized(A * ((A + 1.0) + (A - 1.0) * cs + beta), -2.0 * A * ((A - 1.0) + (A + 1.0) * cs),
			  A * ((A + 1.0) + (A - 1.0) * cs - beta), (A + 1.0) - (A - 1.0) * cs + beta,
			  2.0 * ((A - 1.0) - (A + 1.0) * cs), (A + 1.0) - (A - 1.0) * cs - beta);
}

} // namespace vao::dsp
