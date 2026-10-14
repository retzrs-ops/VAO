// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

#include "calib/auto_settings.h"
#include "calib/measurer.h"
#include "test_util.h"

#include <cstdlib>

using namespace vao::calib;
using vao_test::near;
using vao_test::to_db;

namespace {

// 決まった種で同じノイズを出す
float noise(unsigned &seed)
{
	seed = seed * 1664525u + 1013904223u;
	return (static_cast<float>(seed >> 8) / 8388608.0f) - 1.0f; // -1..1
}

// 無音(ノイズのみ) 3 秒 → 声 12 秒 → 大声 4 秒 を 480 フレームずつ流す
void run_session(Measurer &m, float rate, float noise_amp, float voice_amp, float loud_amp,
		 std::size_t nch)
{
	unsigned seed = 1;
	const std::size_t chunk = static_cast<std::size_t>(rate / 100.0f);
	std::vector<std::vector<float>> bufs(nch, std::vector<float>(chunk));
	std::vector<const float *> ch(nch);
	for (std::size_t c = 0; c < nch; ++c)
		ch[c] = bufs[c].data();

	const int total_chunks = (Measurer::kSilenceSec + Measurer::kSpeechSec + Measurer::kLoudSec) * 100;
	std::size_t n = 0;
	for (int k = 0; k < total_chunks + 50; ++k) {
		const int sec = k / 100;
		float amp = noise_amp;
		if (sec >= Measurer::kSilenceSec)
			amp = voice_amp;
		if (sec >= Measurer::kSilenceSec + Measurer::kSpeechSec)
			amp = loud_amp;
		for (std::size_t c = 0; c < nch; ++c)
			for (std::size_t i = 0; i < chunk; ++i) {
				const float tone = std::sin(6.2831853f * 220.0f * static_cast<float>(n + i) / rate);
				bufs[c][i] = (sec >= Measurer::kSilenceSec ? amp * tone : 0.0f) +
					     (noise_amp * noise(seed));
			}
		n += chunk;
		m.feed(ch.data(), nch, chunk);
	}
}

void test_measurement()
{
	const float rates[] = {44100.0f, 48000.0f};
	for (float rate : rates) {
		for (std::size_t nch : {std::size_t(1), std::size_t(2), std::size_t(8)}) {
			Measurer m;
			m.prepare(rate);
			CHECK(m.phase() == Measurer::Phase::Idle);
			m.start();
			run_session(m, rate, 0.001f, 0.05f, 0.4f, nch);
			CHECK(m.phase() == Measurer::Phase::Done);
			Measurement r;
			CHECK(m.compute(r));
			// ノイズ 0.001 の一様乱数 → 二乗平均 = 0.001^2/3 → 約 -65 dB
			CHECK(near(r.noise_floor_db, -65.0f, 2.0f));
			// 正弦波 0.05 → 二乗平均 0.05^2/2 → 約 -29 dB
			CHECK(near(r.speech_level_db, -29.0f, 1.5f));
			CHECK(near(r.peak_db, to_db(0.4f), 1.0f));
			CHECK(!r.clipped);
		}
	}
}

void test_same_input_same_result()
{
	Measurement a, b;
	for (Measurement *out : {&a, &b}) {
		Measurer m;
		m.prepare(48000.0f);
		m.start();
		run_session(m, 48000.0f, 0.002f, 0.1f, 0.5f, 2);
		CHECK(m.compute(*out));
	}
	CHECK(a.noise_floor_db == b.noise_floor_db);
	CHECK(a.speech_level_db == b.speech_level_db);
	CHECK(a.peak_db == b.peak_db);
}

void test_no_voice_fails()
{
	Measurer m;
	m.prepare(48000.0f);
	m.start();
	run_session(m, 48000.0f, 0.001f, 0.0f, 0.0f, 1);
	CHECK(m.phase() == Measurer::Phase::Done);
	Measurement r;
	CHECK(!m.compute(r));
	CHECK(m.phase() == Measurer::Phase::Failed);
}

void test_clipping_detected()
{
	Measurer m;
	m.prepare(48000.0f);
	m.start();
	run_session(m, 48000.0f, 0.001f, 0.2f, 3.0f, 1); // 大声が 1.0 を超える（音割れ）
	// run_session の大声は正弦波ではなく振幅 3.0 のため、1.0 を超える平らな部分ができる
	Measurement r;
	CHECK(m.compute(r));
	CHECK(r.clipped);
}

void test_restart_and_cancel()
{
	Measurer m;
	m.prepare(48000.0f);
	m.start();
	CHECK(m.phase() == Measurer::Phase::Starting);
	m.cancel();
	CHECK(m.phase() == Measurer::Phase::Idle);
	std::vector<float> buf(480, 0.5f);
	const float *ch[1] = {buf.data()};
	m.feed(ch, 1, 480); // Idle では何もしない
	CHECK(m.phase() == Measurer::Phase::Idle);
	m.start();
	m.feed(ch, 1, 480);
	CHECK(m.phase() == Measurer::Phase::Silence);
	CHECK(m.remaining_sec() >= 1 && m.remaining_sec() <= Measurer::kSilenceSec);
}

void test_auto_settings()
{
	Measurement quiet; // 音量が小さい配信者
	quiet.noise_floor_db = -60.0f;
	quiet.speech_level_db = -38.0f;
	quiet.peak_db = -24.0f;

	const CompSettings c = auto_comp(quiet);
	CHECK(near(c.threshold_db, -44.0f, 1e-3f));
	CHECK(near(c.ratio, 4.0f, 1e-3f));

	const CompState on{true, c.threshold_db, c.ratio};
	const float g = auto_gain_db(quiet, on, -1.0f);
	// 平均 -38 → 減衰 (6*3/4=4.5) → -42.5 から -16 へ → +26.5 dB
	CHECK(near(g, 26.5f, 0.05f));
	// コンプ後のピーク: -44 + 20/4 = -39 → 上限は -1+6+39 = 44 で、制限はかからない
	CHECK(g <= kGainMaxDb);

	// コンプなし: 平均をそのまま目標へ → +22 dB
	const CompState off{false, 0.0f, 1.0f};
	CHECK(near(auto_gain_db(quiet, off, -1.0f), 22.0f, 0.05f));

	// 同じ入力は同じ結果
	CHECK(auto_gain_db(quiet, on, -1.0f) == auto_gain_db(quiet, on, -1.0f));

	// ピークが大きい人は、潰れすぎないようゲインを抑える
	Measurement peaky;
	peaky.speech_level_db = -30.0f;
	peaky.peak_db = -3.0f;
	const float g2 = auto_gain_db(peaky, off, -1.0f);
	CHECK(near(g2, 8.0f, 0.05f)); // 目標は +14 dB だが、上限 -1 + 6 - (-3) = 8 dB に抑える
	CHECK(auto_limiter_ceiling_db() == -1.0f);

	// すでに大きい人はゲインを下げる（範囲内）
	Measurement loud;
	loud.speech_level_db = -8.0f;
	loud.peak_db = -2.0f;
	CHECK(auto_gain_db(loud, off, -1.0f) < 0.0f);
	CHECK(auto_gain_db(loud, off, -1.0f) >= kGainMinDb);
}

} // namespace

int main()
{
	test_measurement();
	test_same_input_same_result();
	test_no_voice_fails();
	test_clipping_detected();
	test_restart_and_cancel();
	test_auto_settings();
	return vao_test::finish();
}
