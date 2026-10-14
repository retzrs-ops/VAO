// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

// AI ノイズ抑制（RNNoise 内蔵）のテスト。

#include "dsp/denoiser.h"
#include "test_util.h"

#include <algorithm>

using vao_test::make_sine;
using vao_test::near;
using vao_test::to_db;

namespace {

const float kRate = 48000.0f;

// 決まった種で同じノイズを出す（-1〜1）
std::vector<float> make_noise(std::size_t frames, float amp, unsigned seed = 1)
{
	std::vector<float> v(frames);
	for (auto &x : v) {
		seed = seed * 1664525u + 1013904223u;
		x = amp * ((static_cast<float>(seed >> 8) / 8388608.0f) - 1.0f);
	}
	return v;
}

float rms_db(const std::vector<float> &v, std::size_t from)
{
	double sum = 0.0;
	for (std::size_t i = from; i < v.size(); ++i)
		sum += static_cast<double>(v[i]) * static_cast<double>(v[i]);
	return 10.0f * std::log10(static_cast<float>(sum / static_cast<double>(v.size() - from)) + 1e-12f);
}

void test_support_and_latency()
{
	vao::dsp::Denoiser d;
	CHECK(d.prepare(kRate));
	CHECK(d.supported());
	CHECK(d.latency_frames() == 480);

	vao::dsp::Denoiser other;
	CHECK(!other.prepare(44100.0f));
	CHECK(!other.supported());
	CHECK(other.latency_frames() == 0);
	// 非対応のレートでは素通し
	const auto src = make_sine(4410, 44100.0f, 0.3f);
	auto buf = src;
	float *ch[1] = {buf.data()};
	other.set_params(true, 1.0f);
	other.process(ch, 1, buf.size());
	CHECK(buf == src);
}

void test_disabled_is_delayed_copy()
{
	vao::dsp::Denoiser d;
	CHECK(d.prepare(kRate));
	d.set_params(false, 1.0f);
	const auto src = make_sine(9600, kRate, 0.3f);
	auto buf = src;
	float *ch[1] = {buf.data()};
	d.process(ch, 1, buf.size());
	const std::size_t lat = d.latency_frames();
	bool zeros = true;
	for (std::size_t i = 0; i < lat; ++i)
		zeros = zeros && buf[i] == 0.0f;
	bool same = true;
	for (std::size_t i = lat; i < buf.size(); ++i)
		same = same && buf[i] == src[i - lat];
	CHECK(zeros);
	CHECK(same);
}

void test_strength_zero_is_delayed_copy()
{
	vao::dsp::Denoiser d;
	CHECK(d.prepare(kRate));
	d.set_params(true, 0.0f);
	const auto src = make_noise(9600, 0.1f);
	auto buf = src;
	float *ch[1] = {buf.data()};
	d.process(ch, 1, buf.size());
	bool same = true;
	for (std::size_t i = d.latency_frames(); i < buf.size(); ++i)
		same = same && buf[i] == src[i - d.latency_frames()];
	CHECK(same);
}

void test_reduces_noise()
{
	// 声のない定常ノイズ（部屋の雑音くらい、約 -59 dB）は大きく減る
	vao::dsp::Denoiser d;
	CHECK(d.prepare(kRate));
	d.set_params(true, 1.0f);
	auto buf = make_noise(48000 * 3, 0.002f);
	const float before = rms_db(buf, 96000);
	float *ch[1] = {buf.data()};
	d.process(ch, 1, buf.size());
	const float after = rms_db(buf, 96000);
	CHECK(after < before - 20.0f);
	CHECK(std::isfinite(after));
}

void test_half_strength_is_in_between()
{
	auto run = [](float strength) {
		vao::dsp::Denoiser d;
		d.prepare(kRate);
		d.set_params(true, strength);
		auto buf = make_noise(48000 * 3, 0.002f);
		float *ch[1] = {buf.data()};
		d.process(ch, 1, buf.size());
		return rms_db(buf, 96000);
	};
	const float full = run(1.0f);
	const float half = run(0.5f);
	const float none = run(0.0f);
	CHECK(full < half && half < none);
}

void test_block_size_independent_and_channels()
{
	const auto src = make_noise(24000, 0.05f, 7);
	vao::dsp::Denoiser da, db;
	da.prepare(kRate);
	db.prepare(kRate);
	da.set_params(true, 1.0f);
	db.set_params(true, 1.0f);
	auto a = src;
	auto b = src;
	float *cha[1] = {a.data()};
	da.process(cha, 1, a.size());
	for (std::size_t pos = 0; pos < b.size(); pos += 77) {
		const std::size_t n = std::min<std::size_t>(77, b.size() - pos);
		float *chb[1] = {b.data() + pos};
		db.process(chb, 1, n);
	}
	CHECK(a == b);

	for (std::size_t nch = 1; nch <= 8; ++nch) {
		vao::dsp::Denoiser d;
		d.prepare(kRate);
		d.set_params(true, 1.0f);
		std::vector<std::vector<float>> bufs(nch, src);
		std::vector<float *> chs(nch);
		for (std::size_t c = 0; c < nch; ++c)
			chs[c] = bufs[c].data();
		d.process(chs.data(), nch, src.size());
		bool ok = true;
		for (std::size_t c = 0; c < nch; ++c)
			ok = ok && bufs[c] == bufs[0];
		for (float x : bufs[0])
			ok = ok && std::isfinite(x);
		CHECK(ok); // 同じ入力ならチャンネルごとに同じ結果
	}
}

void test_edge_cases()
{
	vao::dsp::Denoiser d;
	d.prepare(kRate);
	d.set_params(true, NAN);
	d.process(nullptr, 2, 100);
	float x = 0.5f;
	float *ch[1] = {&x};
	d.process(ch, 1, 0);
	CHECK(x == 0.5f);
	float *with_null[2] = {nullptr, &x};
	d.process(with_null, 2, 1);
	CHECK(std::isfinite(x));
	// 9 チャンネルは素通し
	std::vector<float> v(9, 0.25f);
	std::vector<float *> nine(9);
	for (std::size_t c = 0; c < 9; ++c)
		nine[c] = &v[c];
	d.process(nine.data(), 9, 1);
	CHECK(v[0] == 0.25f);
}

} // namespace

int main()
{
	test_support_and_latency();
	test_disabled_is_delayed_copy();
	test_strength_zero_is_delayed_copy();
	test_reduces_noise();
	test_half_strength_is_in_between();
	test_block_size_independent_and_channels();
	test_edge_cases();
	return vao_test::finish();
}
