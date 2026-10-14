// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

// dsp/gain のテスト。OBS なしで動く。音はコードで作る（音声ファイルは使わない）。

#include "dsp/gain.h"
#include "test_util.h"

#include <cmath>
#include <cstdio>
#include <vector>

namespace {

using vao_test::near;

// 正弦波（振幅 0.5）
std::vector<float> make_sine(std::size_t frames, float rate, float hz = 440.0f)
{
	std::vector<float> v(frames);
	for (std::size_t i = 0; i < frames; ++i)
		v[i] = 0.5f * std::sin(6.2831853f * hz * static_cast<float>(i) / rate);
	return v;
}

void test_db_to_linear()
{
	CHECK(near(vao::dsp::db_to_linear(0.0f), 1.0f, 1e-6f));
	CHECK(near(vao::dsp::db_to_linear(6.0206f), 2.0f, 1e-3f));
	CHECK(near(vao::dsp::db_to_linear(-6.0206f), 0.5f, 1e-3f));
	CHECK(vao::dsp::db_to_linear(-120.0f) == 0.0f);
	CHECK(vao::dsp::db_to_linear(-1000.0f) == 0.0f);
	CHECK(vao::dsp::db_to_linear(NAN) == 0.0f);
	CHECK(std::isfinite(vao::dsp::db_to_linear(1000.0f)));
	CHECK(vao::dsp::db_to_linear(1000.0f) == vao::dsp::db_to_linear(vao::dsp::kMaxGainDb));
}

void test_unity_is_bit_exact()
{
	const auto src = make_sine(480, 48000.0f);
	auto buf = src;
	float *ch[1] = {buf.data()};
	vao::dsp::Gain g;
	g.process(ch, 1, buf.size());
	CHECK(buf == src);
}

void test_fixed_gain()
{
	const auto src = make_sine(480, 48000.0f);
	auto buf = src;
	float *ch[1] = {buf.data()};
	vao::dsp::Gain g;
	g.set_gain_db(-6.0206f);
	g.reset();
	g.process(ch, 1, buf.size());
	bool ok = true;
	for (std::size_t i = 0; i < buf.size(); ++i)
		ok = ok && near(buf[i], src[i] * 0.5f, 1e-4f);
	CHECK(ok);
	CHECK(near(g.gain_db(), -6.0206f, 1e-6f));
}

void test_ramp_is_smooth_and_reaches_target()
{
	std::vector<float> buf(480, 1.0f); // 一定値に掛けると倍率の変化が見える
	float *ch[1] = {buf.data()};
	vao::dsp::Gain g;
	g.set_gain_db(-6.0206f); // 現在 1.0 → 目標 0.5
	g.process(ch, 1, buf.size());
	bool monotonic = true;
	for (std::size_t i = 1; i < buf.size(); ++i)
		monotonic = monotonic && buf[i] <= buf[i - 1];
	CHECK(monotonic);
	CHECK(buf.front() < 1.0f);
	CHECK(near(buf.back(), 0.5f, 1e-3f));

	// 次のブロックは一定
	std::vector<float> next(480, 1.0f);
	float *ch2[1] = {next.data()};
	g.process(ch2, 1, next.size());
	bool flat = true;
	for (float x : next)
		flat = flat && near(x, 0.5f, 1e-3f);
	CHECK(flat);
}

void test_mute()
{
	auto buf = make_sine(480, 48000.0f);
	float *ch[1] = {buf.data()};
	vao::dsp::Gain g;
	g.set_gain_db(-200.0f);
	g.reset();
	g.process(ch, 1, buf.size());
	bool silent = true;
	for (float x : buf)
		silent = silent && x == 0.0f;
	CHECK(silent);
}

void test_channels_and_rates()
{
	const float rates[] = {44100.0f, 48000.0f};
	for (float rate : rates) {
		const std::size_t frames = static_cast<std::size_t>(rate / 100.0f); // 10ms
		for (std::size_t nch = 1; nch <= 8; ++nch) {
			std::vector<std::vector<float>> bufs(nch, make_sine(frames, rate));
			std::vector<float *> ch(nch);
			for (std::size_t c = 0; c < nch; ++c)
				ch[c] = bufs[c].data();
			vao::dsp::Gain g;
			g.set_gain_db(6.0206f);
			g.reset();
			g.process(ch.data(), nch, frames);
			bool ok = true;
			for (std::size_t c = 0; c < nch; ++c)
				for (std::size_t i = 0; i < frames; ++i)
					ok = ok && std::isfinite(bufs[c][i]) &&
					     near(bufs[c][i], make_sine(frames, rate)[i] * 2.0f, 1e-3f);
			CHECK(ok);
		}
	}
}

void test_edge_cases()
{
	vao::dsp::Gain g;
	g.set_gain_db(3.0f);
	g.process(nullptr, 2, 100); // 落ちないこと
	float dummy = 1.0f;
	float *ch[1] = {&dummy};
	g.process(ch, 1, 0); // 0 フレームでは何も変わらない
	CHECK(dummy == 1.0f);

	float *with_null[2] = {nullptr, &dummy};
	g.reset();
	g.process(with_null, 2, 1); // nullptr のチャンネルは飛ばす
	CHECK(dummy > 1.0f);

	g.set_gain_db(NAN); // NaN は無視して直前の値を保つ
	CHECK(near(g.gain_db(), 3.0f, 1e-6f));
}

} // namespace

int main()
{
	test_db_to_linear();
	test_unity_is_bit_exact();
	test_fixed_gain();
	test_ramp_is_smooth_and_reaches_target();
	test_mute();
	test_channels_and_rates();
	test_edge_cases();
	return vao_test::finish();
}
