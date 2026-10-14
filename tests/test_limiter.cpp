// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

#include "dsp/limiter.h"
#include "test_util.h"

#include <algorithm>

using vao_test::make_sine;
using vao_test::near;

namespace {

const float kCeilDb = -1.0f;
const float kCeil = 0.8912509f; // -1 dBFS

void test_never_exceeds_ceiling()
{
	const float rates[] = {44100.0f, 48000.0f};
	for (float rate : rates) {
		for (std::size_t nch = 1; nch <= 8; ++nch) {
			// 大音量の正弦波（+6 dB）に、突然の巨大なピークを混ぜる
			std::vector<std::vector<float>> bufs(nch, make_sine(static_cast<std::size_t>(rate), rate, 2.0f));
			for (std::size_t c = 0; c < nch; ++c)
				bufs[c][20000] = 8.0f;
			std::vector<float *> ch(nch);
			for (std::size_t c = 0; c < nch; ++c)
				ch[c] = bufs[c].data();
			vao::dsp::Limiter lim;
			lim.prepare(rate);
			lim.set_params(true, kCeilDb);
			lim.process(ch.data(), nch, static_cast<std::size_t>(rate));
			float peak = 0.0f;
			for (std::size_t c = 0; c < nch; ++c)
				for (float x : bufs[c])
					peak = std::max(peak, std::fabs(x));
			CHECK(peak <= kCeil + 1e-6f);
		}
	}
}

void test_quiet_passes_with_delay()
{
	const float rate = 48000.0f;
	const auto src = make_sine(4800, rate, 0.3f);
	auto buf = src;
	float *ch[1] = {buf.data()};
	vao::dsp::Limiter lim;
	lim.prepare(rate);
	lim.set_params(true, kCeilDb);
	lim.process(ch, 1, buf.size());
	const std::size_t d = lim.latency_frames();
	CHECK(d > 0 && d < 200);
	bool same = true;
	for (std::size_t i = d; i < buf.size(); ++i)
		same = same && near(buf[i], src[i - d], 1e-6f);
	CHECK(same);
}

void test_disabled_delays_but_does_not_limit()
{
	const float rate = 48000.0f;
	const auto src = make_sine(4800, rate, 1.5f);
	auto buf = src;
	float *ch[1] = {buf.data()};
	vao::dsp::Limiter lim;
	lim.prepare(rate);
	lim.set_params(false, kCeilDb);
	lim.process(ch, 1, buf.size());
	const std::size_t d = lim.latency_frames();
	bool same = true;
	for (std::size_t i = d; i < buf.size(); ++i)
		same = same && near(buf[i], src[i - d], 1e-6f);
	CHECK(same);
}

void test_block_size_independent()
{
	const float rate = 48000.0f;
	const auto src = make_sine(9600, rate, 2.0f);
	auto a = src;
	auto b = src;
	vao::dsp::Limiter la, lb;
	la.prepare(rate);
	lb.prepare(rate);
	la.set_params(true, kCeilDb);
	lb.set_params(true, kCeilDb);
	float *cha[1] = {a.data()};
	la.process(cha, 1, a.size());
	for (std::size_t pos = 0; pos < b.size(); pos += 53) {
		const std::size_t n = std::min<std::size_t>(53, b.size() - pos);
		float *chb[1] = {b.data() + pos};
		lb.process(chb, 1, n);
	}
	CHECK(a == b);
}

void test_edge_cases()
{
	vao::dsp::Limiter lim;
	lim.prepare(48000.0f);
	lim.set_params(true, kCeilDb);
	lim.process(nullptr, 2, 100);
	float x = 3.0f;
	float *ch[1] = {&x};
	lim.process(ch, 1, 0);
	CHECK(x == 3.0f);
	float *with_null[2] = {nullptr, &x};
	lim.process(with_null, 2, 1);
	CHECK(std::isfinite(x));
	// 9 チャンネルは素通し（確保を避けるため）
	std::vector<float> v(9, 5.0f);
	std::vector<float *> nine(9);
	for (std::size_t c = 0; c < 9; ++c)
		nine[c] = &v[c];
	lim.process(nine.data(), 9, 1);
	CHECK(v[0] == 5.0f);
}

} // namespace

int main()
{
	test_never_exceeds_ceiling();
	test_quiet_passes_with_delay();
	test_disabled_delays_but_does_not_limit();
	test_block_size_independent();
	test_edge_cases();
	return vao_test::finish();
}
