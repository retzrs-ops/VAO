// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

// ハイパス・ノイズゲート・EQ・ディエッサーのテスト。

#include "dsp/deesser.h"
#include "dsp/eq.h"
#include "dsp/gate.h"
#include "dsp/highpass.h"
#include "test_util.h"

#include <algorithm>

using vao_test::make_sine;
using vao_test::near;
using vao_test::to_db;

namespace {

const float kRate = 48000.0f;

// 後半(落ち着いたあと)のピーク
float tail_peak(const std::vector<float> &v)
{
	float p = 0.0f;
	for (std::size_t i = v.size() / 2; i < v.size(); ++i)
		p = std::max(p, std::fabs(v[i]));
	return p;
}

// 1 チャンネルの音をフィルタに通して、後半のピークの変化(dB)を返す
template<class Filter> float gain_db_at(Filter &f, float hz, float amp = 0.1f, std::size_t frames = 48000)
{
	auto buf = make_sine(frames, kRate, amp, hz);
	float *ch[1] = {buf.data()};
	f.process(ch, 1, frames);
	return to_db(tail_peak(buf)) - to_db(amp);
}

void test_highpass()
{
	vao::dsp::HighPass hp;
	hp.prepare(kRate);
	hp.set_params(true, 80.0f);
	CHECK(gain_db_at(hp, 30.0f) < -20.0f);
	hp.reset();
	CHECK(near(gain_db_at(hp, 1000.0f), 0.0f, 0.3f));

	// オフは音を変えない
	vao::dsp::HighPass off;
	off.prepare(kRate);
	off.set_params(false, 80.0f);
	const auto src = make_sine(4800, kRate, 0.5f, 40.0f);
	auto buf = src;
	float *ch[1] = {buf.data()};
	off.process(ch, 1, buf.size());
	CHECK(buf == src);

	// 1〜8ch でも壊れない
	for (std::size_t nch = 1; nch <= 8; ++nch) {
		vao::dsp::HighPass h;
		h.prepare(44100.0f);
		h.set_params(true, 100.0f);
		std::vector<std::vector<float>> bufs(nch, make_sine(4410, 44100.0f, 0.3f, 60.0f));
		std::vector<float *> chs(nch);
		for (std::size_t c = 0; c < nch; ++c)
			chs[c] = bufs[c].data();
		h.process(chs.data(), nch, 4410);
		bool ok = true;
		for (std::size_t c = 0; c < nch; ++c)
			ok = ok && std::isfinite(bufs[c][4000]) && bufs[c] == bufs[0];
		CHECK(ok);
	}
	vao::dsp::HighPass e;
	e.prepare(kRate);
	e.set_params(true, 80.0f);
	e.process(nullptr, 2, 10);
	float x = 1.0f;
	float *chx[1] = {&x};
	e.process(chx, 1, 0);
	CHECK(x == 1.0f);
}

void test_gate()
{
	// 小さい雑音(二乗平均 約 -63 dB)はしきい値 -50 dB 未満 → 閉じる
	{
		vao::dsp::NoiseGate g;
		g.prepare(kRate);
		g.set_params(true, -50.0f);
		auto buf = make_sine(96000, kRate, 0.002f, 500.0f);
		float *ch[1] = {buf.data()};
		g.process(ch, 1, buf.size());
		CHECK(to_db(tail_peak(buf)) < to_db(0.002f) - 30.0f);
	}
	// 声(二乗平均 約 -23 dB)は通る
	{
		vao::dsp::NoiseGate g;
		g.prepare(kRate);
		g.set_params(true, -50.0f);
		auto buf = make_sine(48000, kRate, 0.1f, 300.0f);
		float *ch[1] = {buf.data()};
		g.process(ch, 1, buf.size());
		CHECK(near(to_db(tail_peak(buf)), to_db(0.1f), 0.3f));
	}
	// 声のあとの無音で閉じ、声が戻ると素早く開く
	{
		vao::dsp::NoiseGate g;
		g.prepare(kRate);
		g.set_params(true, -50.0f);
		std::vector<float> buf;
		auto voice = make_sine(24000, kRate, 0.1f, 300.0f);
		buf.insert(buf.end(), voice.begin(), voice.end());
		buf.insert(buf.end(), 48000, 0.0f);
		buf.insert(buf.end(), voice.begin(), voice.end());
		std::vector<float> in = buf;
		float *ch[1] = {buf.data()};
		g.process(ch, 1, buf.size());
		// 無音の終わりのゲインは小さい（無音なので出力ゼロ。入力が 0 でも壊れない）
		CHECK(std::isfinite(buf[24000 + 47999]));
		// 戻った声: 20 ms 後には 9 割以上通っている
		const std::size_t start = 24000 + 48000;
		float p = 0.0f;
		for (std::size_t i = start + 960; i < start + 2400; ++i)
			p = std::max(p, std::fabs(buf[i]));
		CHECK(p > 0.09f);
	}
	// オフは音を変えない
	{
		vao::dsp::NoiseGate g;
		g.prepare(kRate);
		g.set_params(false, -50.0f);
		const auto src = make_sine(4800, kRate, 0.001f);
		auto buf = src;
		float *ch[1] = {buf.data()};
		g.process(ch, 1, buf.size());
		CHECK(buf == src);
	}
	// ブロックの切り方に依らない
	{
		vao::dsp::NoiseGate ga, gb;
		ga.prepare(kRate);
		gb.prepare(kRate);
		ga.set_params(true, -40.0f);
		gb.set_params(true, -40.0f);
		auto a = make_sine(24000, kRate, 0.01f);
		auto b = a;
		float *cha[1] = {a.data()};
		ga.process(cha, 1, a.size());
		for (std::size_t pos = 0; pos < b.size(); pos += 41) {
			const std::size_t n = std::min<std::size_t>(41, b.size() - pos);
			float *chb[1] = {b.data() + pos};
			gb.process(chb, 1, n);
		}
		CHECK(a == b);
	}
}

void test_eq()
{
	// すべて 0 dB は音を変えない
	{
		vao::dsp::Eq eq;
		eq.prepare(kRate);
		eq.set_params(true, vao::dsp::EqGains{});
		const auto src = make_sine(4800, kRate, 0.5f);
		auto buf = src;
		float *ch[1] = {buf.data()};
		eq.process(ch, 1, buf.size());
		CHECK(buf == src);
	}
	// 明瞭さ +6 dB
	{
		vao::dsp::Eq eq;
		eq.prepare(kRate);
		eq.set_params(true, vao::dsp::EqGains{0.0f, 0.0f, 6.0f, 0.0f});
		CHECK(near(gain_db_at(eq, 3000.0f), 6.0f, 0.5f));
		eq.reset();
		CHECK(near(gain_db_at(eq, 100.0f), 0.0f, 0.7f)); // 離れた帯域はほぼ変わらない
	}
	// こもり -6 dB
	{
		vao::dsp::Eq eq;
		eq.prepare(kRate);
		eq.set_params(true, vao::dsp::EqGains{0.0f, -6.0f, 0.0f, 0.0f});
		CHECK(near(gain_db_at(eq, 300.0f), -6.0f, 0.5f));
	}
	// 低域シェルフ・空気感シェルフ
	{
		vao::dsp::Eq eq;
		eq.prepare(kRate);
		eq.set_params(true, vao::dsp::EqGains{6.0f, 0.0f, 0.0f, 6.0f});
		CHECK(near(gain_db_at(eq, 40.0f), 6.0f, 1.0f));
		eq.reset();
		CHECK(near(gain_db_at(eq, 18000.0f), 6.0f, 1.0f));
	}
	// オフは音を変えない
	{
		vao::dsp::Eq eq;
		eq.prepare(kRate);
		eq.set_params(false, vao::dsp::EqGains{0.0f, 0.0f, 9.0f, 0.0f});
		const auto src = make_sine(4800, kRate, 0.5f, 3000.0f);
		auto buf = src;
		float *ch[1] = {buf.data()};
		eq.process(ch, 1, buf.size());
		CHECK(buf == src);
	}
	// プリセット
	CHECK(vao::dsp::kEqPresetCount == 6);
	const auto natural = vao::dsp::eq_preset(0);
	CHECK(natural.low_db == 0.0f && natural.mud_db == 0.0f && natural.pres_db == 0.0f && natural.air_db == 0.0f);
	CHECK(vao::dsp::eq_preset(99).pres_db == 0.0f);
	CHECK(vao::dsp::eq_preset(-1).pres_db == 0.0f);
	CHECK(vao::dsp::eq_preset(1).pres_db > 0.0f); // クリアは明瞭さを足す
	CHECK(vao::dsp::eq_preset(2).low_db > 0.0f);  // あたたかいは低域を足す
	for (int i = 0; i < vao::dsp::kEqPresetCount; ++i) {
		const auto p = vao::dsp::eq_preset(i);
		CHECK(std::fabs(p.low_db) <= vao::dsp::kEqMaxDb && std::fabs(p.mud_db) <= vao::dsp::kEqMaxDb &&
		      std::fabs(p.pres_db) <= vao::dsp::kEqMaxDb && std::fabs(p.air_db) <= vao::dsp::kEqMaxDb);
	}
	// 1〜8ch、NaN の設定でも壊れない
	for (std::size_t nch = 1; nch <= 8; ++nch) {
		vao::dsp::Eq eq;
		eq.prepare(44100.0f);
		eq.set_params(true, vao::dsp::EqGains{NAN, 3.0f, -3.0f, 100.0f});
		std::vector<std::vector<float>> bufs(nch, make_sine(4410, 44100.0f, 0.3f, 1000.0f));
		std::vector<float *> chs(nch);
		for (std::size_t c = 0; c < nch; ++c)
			chs[c] = bufs[c].data();
		eq.process(chs.data(), nch, 4410);
		bool ok = true;
		for (std::size_t c = 0; c < nch; ++c)
			ok = ok && std::isfinite(bufs[c][4000]) && bufs[c] == bufs[0];
		CHECK(ok);
	}
}

void test_deesser()
{
	// 7 kHz の強い音(帯域の二乗平均 約 -17 dB)はしきい値 -30 dB を超える → 下がる
	{
		vao::dsp::Deesser d;
		d.prepare(kRate);
		d.set_params(true, -30.0f);
		CHECK(gain_db_at(d, 7000.0f, 0.3f) < -4.0f);
	}
	// 300 Hz の声はほぼ変わらない
	{
		vao::dsp::Deesser d;
		d.prepare(kRate);
		d.set_params(true, -30.0f);
		CHECK(near(gain_db_at(d, 300.0f, 0.3f), 0.0f, 0.5f));
	}
	// しきい値より小さいサ行は変わらない
	{
		vao::dsp::Deesser d;
		d.prepare(kRate);
		d.set_params(true, -30.0f);
		CHECK(near(gain_db_at(d, 7000.0f, 0.005f), 0.0f, 0.3f));
	}
	// 下げすぎない（最大 15 dB）
	{
		vao::dsp::Deesser d;
		d.prepare(kRate);
		d.set_params(true, -80.0f);
		CHECK(gain_db_at(d, 7000.0f, 0.5f) > -16.0f);
	}
	// オフは音を変えない
	{
		vao::dsp::Deesser d;
		d.prepare(kRate);
		d.set_params(false, -80.0f);
		const auto src = make_sine(4800, kRate, 0.5f, 7000.0f);
		auto buf = src;
		float *ch[1] = {buf.data()};
		d.process(ch, 1, buf.size());
		CHECK(buf == src);
	}
	// 1〜8ch で壊れない・連動する
	for (std::size_t nch = 1; nch <= 8; ++nch) {
		vao::dsp::Deesser d;
		d.prepare(44100.0f);
		d.set_params(true, -40.0f);
		std::vector<std::vector<float>> bufs(nch, make_sine(4410, 44100.0f, 0.3f, 7000.0f));
		std::vector<float *> chs(nch);
		for (std::size_t c = 0; c < nch; ++c)
			chs[c] = bufs[c].data();
		d.process(chs.data(), nch, 4410);
		bool ok = true;
		for (std::size_t c = 0; c < nch; ++c)
			ok = ok && std::isfinite(bufs[c][4000]) && bufs[c] == bufs[0];
		CHECK(ok);
	}
}

} // namespace

int main()
{
	test_highpass();
	test_gate();
	test_eq();
	test_deesser();
	return vao_test::finish();
}
