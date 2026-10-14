// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

#include "dsp/compressor.h"
#include "test_util.h"

#include <algorithm>

using vao_test::make_sine;
using vao_test::near;
using vao_test::to_db;

namespace {

float tail_peak(const std::vector<float> &v)
{
	float p = 0.0f;
	for (std::size_t i = v.size() / 2; i < v.size(); ++i)
		p = std::max(p, std::fabs(v[i]));
	return p;
}

void test_reduces_loud_signal()
{
	const float rate = 48000.0f;
	auto buf = make_sine(48000, rate, 0.5f); // -6 dBFS
	float *ch[1] = {buf.data()};
	vao::dsp::Compressor c;
	c.prepare(rate);
	c.set_params(true, -20.0f, 4.0f);
	c.process(ch, 1, buf.size());
	// 超過 14 dB を 4:1 → 減衰 10.5 dB → 約 -16.5 dBFS
	CHECK(near(to_db(tail_peak(buf)), -16.5f, 2.0f));
}

void test_below_threshold_untouched()
{
	const float rate = 48000.0f;
	const auto src = make_sine(24000, rate, 0.05f); // -26 dBFS
	auto buf = src;
	float *ch[1] = {buf.data()};
	vao::dsp::Compressor c;
	c.prepare(rate);
	c.set_params(true, -20.0f, 4.0f);
	c.process(ch, 1, buf.size());
	bool same = true;
	for (std::size_t i = 0; i < buf.size(); ++i)
		same = same && near(buf[i], src[i], 1e-5f);
	CHECK(same);
}

void test_disabled_is_bit_exact()
{
	const float rate = 44100.0f;
	const auto src = make_sine(10000, rate, 0.9f);
	auto buf = src;
	float *ch[1] = {buf.data()};
	vao::dsp::Compressor c;
	c.prepare(rate);
	c.set_params(false, -30.0f, 10.0f);
	c.process(ch, 1, buf.size());
	CHECK(buf == src);
}

void test_block_size_independent()
{
	const float rate = 48000.0f;
	const auto src = make_sine(9600, rate, 0.7f);
	auto a = src;
	auto b = src;
	vao::dsp::Compressor ca, cb;
	ca.prepare(rate);
	cb.prepare(rate);
	ca.set_params(true, -18.0f, 3.0f);
	cb.set_params(true, -18.0f, 3.0f);
	float *cha[1] = {a.data()};
	ca.process(cha, 1, a.size());
	for (std::size_t pos = 0; pos < b.size(); pos += 37) {
		const std::size_t n = std::min<std::size_t>(37, b.size() - pos);
		float *chb[1] = {b.data() + pos};
		cb.process(chb, 1, n);
	}
	CHECK(a == b);
}

void test_channels_linked_and_safe()
{
	const float rate = 48000.0f;
	for (std::size_t nch = 1; nch <= 8; ++nch) {
		std::vector<std::vector<float>> bufs(nch, make_sine(4800, rate, 0.6f));
		std::vector<float *> ch(nch);
		for (std::size_t c = 0; c < nch; ++c)
			ch[c] = bufs[c].data();
		vao::dsp::Compressor comp;
		comp.prepare(rate);
		comp.set_params(true, -20.0f, 3.0f);
		comp.process(ch.data(), nch, 4800);
		bool ok = true;
		for (std::size_t c = 0; c < nch; ++c)
			ok = ok && bufs[c] == bufs[0] && std::isfinite(bufs[c][100]);
		CHECK(ok); // 同じ入力なら同じ出力（連動している）
	}
	vao::dsp::Compressor comp;
	comp.prepare(48000.0f);
	comp.set_params(true, -20.0f, 3.0f);
	comp.process(nullptr, 2, 100);
	float x = 0.5f;
	float *ch[1] = {&x};
	comp.process(ch, 1, 0);
	CHECK(x == 0.5f);
	float *with_null[2] = {nullptr, &x};
	comp.process(with_null, 2, 1);
	CHECK(std::isfinite(x));
}

} // namespace

int main()
{
	test_reduces_loud_signal();
	test_below_threshold_untouched();
	test_disabled_is_bit_exact();
	test_block_size_independent();
	test_channels_linked_and_safe();
	return vao_test::finish();
}
