// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

// テスト共通の小さな道具。各テストは main() の最後で return vao_test::finish(); とする。

#pragma once

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <vector>

namespace vao_test {

inline int &pass_count()
{
	static int n = 0;
	return n;
}
inline int &fail_count()
{
	static int n = 0;
	return n;
}

inline int finish()
{
	std::printf("PASS %d / FAIL %d\n", pass_count(), fail_count());
	return fail_count() == 0 ? 0 : 1;
}

inline bool near(float a, float b, float tol)
{
	return std::fabs(a - b) <= tol;
}

// 正弦波
inline std::vector<float> make_sine(std::size_t frames, float rate, float amp, float hz = 440.0f)
{
	std::vector<float> v(frames);
	for (std::size_t i = 0; i < frames; ++i)
		v[i] = amp * std::sin(6.2831853f * hz * static_cast<float>(i) / rate);
	return v;
}

inline float to_db(float x)
{
	return 20.0f * std::log10(x + 1e-12f);
}

} // namespace vao_test

#define CHECK(cond)                                                                       \
	do {                                                                              \
		if (cond) {                                                               \
			++vao_test::pass_count();                                         \
		} else {                                                                  \
			++vao_test::fail_count();                                         \
			std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);       \
		}                                                                         \
	} while (0)
