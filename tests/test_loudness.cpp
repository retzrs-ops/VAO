// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

// ラウドネス（LUFS）の K 特性フィルタのテスト。規格書の係数と、基準の音の値で確かめる。

#include "dsp/loudness.h"
#include "test_util.h"

using vao_test::make_sine;
using vao_test::near;

namespace {

void test_coefficients_match_standard_at_48k()
{
	// ITU-R BS.1770 が 48 kHz 用に載せている係数
	const auto pre = vao::dsp::kweight_prefilter(48000.0);
	CHECK(std::fabs(pre.b0 - 1.53512485958697) < 1e-6);
	CHECK(std::fabs(pre.b1 - (-2.69169618940638)) < 1e-6);
	CHECK(std::fabs(pre.b2 - 1.19839281085285) < 1e-6);
	CHECK(std::fabs(pre.a1 - (-1.69065929318241)) < 1e-6);
	CHECK(std::fabs(pre.a2 - 0.73248077421585) < 1e-6);

	const auto rlb = vao::dsp::kweight_rlb(48000.0);
	CHECK(rlb.b0 == 1.0 && rlb.b1 == -2.0 && rlb.b2 == 1.0);
	CHECK(std::fabs(rlb.a1 - (-1.99004745483398)) < 1e-6);
	CHECK(std::fabs(rlb.a2 - 0.99007225036621) < 1e-6);
}

// 1 秒ぶんを流して、LUFS を返す（チャンネルごとの二乗平均を足し合わせる）
float lufs_of(const std::vector<std::vector<float>> &chs, double rate)
{
	double total = 0.0;
	for (const auto &x : chs) {
		vao::dsp::Biquad pre, rlb;
		pre.set_coefs(vao::dsp::kweight_prefilter(rate));
		rlb.set_coefs(vao::dsp::kweight_rlb(rate));
		double sum = 0.0;
		std::size_t n = 0;
		for (std::size_t i = 0; i < x.size(); ++i) {
			const float y = rlb.process(pre.process(x[i]));
			if (i < x.size() / 4) // 立ち上がりは数えない
				continue;
			sum += static_cast<double>(y) * static_cast<double>(y);
			++n;
		}
		total += sum / static_cast<double>(n);
	}
	return vao::dsp::kLufsOffsetDb + 10.0f * std::log10(static_cast<float>(total));
}

void test_reference_tone()
{
	// 基準: 1 kHz の正弦波（ピーク -20 dBFS）を左右に流すと、約 -20 LUFS
	for (double rate : {44100.0, 48000.0}) {
		const auto s = make_sine(static_cast<std::size_t>(rate) * 2, static_cast<float>(rate), 0.1f, 1000.0f);
		CHECK(near(lufs_of({s, s}, rate), -20.0f, 0.15f));
		// 1 チャンネルだけなら、-3 dB（チャンネルを足し合わせるため）
		CHECK(near(lufs_of({s}, rate), -23.01f, 0.15f));
	}
}

void test_weighting_shape()
{
	const double rate = 48000.0;
	auto level = [&](float hz) {
		const auto s = make_sine(static_cast<std::size_t>(rate) * 2, static_cast<float>(rate), 0.1f, hz);
		return lufs_of({s}, rate);
	};
	const float at_1k = level(1000.0f);
	CHECK(level(10000.0f) > at_1k + 3.0f);          // 高い音は持ち上がる（約 +4 dB）
	CHECK(level(20.0f) < at_1k - 6.0f);             // 低い音は切られる
	CHECK(near(level(200.0f), at_1k - 0.3f, 0.7f)); // 声の低い帯域は、ほぼそのまま
	// 音の大きさが 10 dB 変われば、LUFS も 10 dB 変わる
	const auto loud = make_sine(static_cast<std::size_t>(rate) * 2, static_cast<float>(rate), 0.316228f, 1000.0f);
	CHECK(near(lufs_of({loud}, rate), at_1k + 10.0f, 0.05f));
}

} // namespace

int main()
{
	test_coefficients_match_standard_at_48k();
	test_reference_tone();
	test_weighting_shape();
	return vao_test::finish();
}
