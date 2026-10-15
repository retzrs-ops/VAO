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
void run_session(Measurer &m, float rate, float noise_amp, float voice_amp, float loud_amp, std::size_t nch,
		 float hz = 220.0f, float kbd_amp = 0.0f, float mouse_amp = 0.0f)
{
	unsigned seed = 1;
	const std::size_t chunk = static_cast<std::size_t>(rate / 100.0f);
	std::vector<std::vector<float>> bufs(nch, std::vector<float>(chunk));
	std::vector<const float *> ch(nch);
	for (std::size_t c = 0; c < nch; ++c)
		ch[c] = bufs[c].data();

	const int t_kbd = Measurer::kSilenceSec;
	const int t_mouse = t_kbd + Measurer::kKeyboardSec;
	const int t_speech = t_mouse + Measurer::kMouseSec;
	const int t_loud = t_speech + Measurer::kSpeechSec;
	const int total_chunks = (t_loud + Measurer::kLoudSec) * 100;
	std::size_t n = 0;
	for (int k = 0; k < total_chunks + 50; ++k) {
		const int sec = k / 100;
		float amp = 0.0f;
		if (sec >= t_speech)
			amp = voice_amp;
		if (sec >= t_loud)
			amp = loud_amp;
		// 0.25 秒ごとに 10 ms のクリック音（雑音状のバースト）
		float click_amp = 0.0f;
		if (k % 25 == 0) {
			if (sec >= t_kbd && sec < t_mouse)
				click_amp = kbd_amp;
			else if (sec >= t_mouse && sec < t_speech)
				click_amp = mouse_amp;
		}
		for (std::size_t c = 0; c < nch; ++c)
			for (std::size_t i = 0; i < chunk; ++i) {
				const float tone = std::sin(6.2831853f * hz * static_cast<float>(n + i) / rate);
				bufs[c][i] = (sec >= t_speech ? amp * tone : 0.0f) + (noise_amp * noise(seed)) +
					     click_amp * noise(seed);
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
			// ラウドネス: -0.691 + チャンネル数ぶんの足し合わせ(10*log10(n)) + K 特性（220 Hz ではほぼ 0 dB）
			const float expected_lufs =
				-29.03f - 0.691f + 10.0f * std::log10(static_cast<float>(nch)) + 0.07f;
			CHECK(near(r.speech_lufs, expected_lufs, 0.7f));
			CHECK(near(r.peak_db, to_db(0.4f), 1.0f));
			CHECK(!r.clipped);
			// 220 Hz の声は、こもり(300 Hz)の帯域が明瞭さ(3 kHz)の帯域よりずっと強い
			CHECK(r.band_mud_db > r.band_pres_db + 20.0f);
			CHECK(r.band_mud_db > r.band_sib_db + 20.0f);
		}
	}
}

void test_click_sampling()
{
	// クリック音なし: キーボード・マウスの値は部屋のノイズ程度で、「あり」とは判定されない
	{
		Measurer m;
		m.prepare(48000.0f);
		m.start();
		run_session(m, 48000.0f, 0.001f, 0.05f, 0.4f, 1);
		Measurement r;
		CHECK(m.compute(r));
		CHECK(r.keyboard_level_db < r.noise_floor_db + 6.0f);
		CHECK(r.mouse_level_db < r.noise_floor_db + 6.0f);
		CHECK(!clicks_present(r));
	}
	// クリック音あり: 10 ms バーストの強さが測れる（キーボードのほうが大きい）
	{
		Measurer m;
		m.prepare(48000.0f);
		m.start();
		run_session(m, 48000.0f, 0.001f, 0.05f, 0.4f, 1, 220.0f, 0.05f, 0.02f);
		Measurement r;
		CHECK(m.compute(r));
		CHECK(near(r.keyboard_level_db, -32.8f, 2.0f));
		CHECK(near(r.mouse_level_db, -40.8f, 2.0f));
		CHECK(r.keyboard_level_db > r.mouse_level_db + 5.0f);
		CHECK(clicks_present(r));
		// 声の測定は、クリック音の段階に影響されない
		CHECK(near(r.speech_level_db, -29.0f, 1.5f));
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
	quiet.speech_lufs = -35.0f; // ステレオ（2 チャンネル）なので、二乗平均より 3 dB 大きい
	quiet.peak_db = -24.0f;

	const CompSettings c = auto_comp(quiet);
	CHECK(near(c.threshold_db, -44.0f, 1e-3f));
	CHECK(near(c.ratio, 4.0f, 1e-3f));

	const CompState on{true, c.threshold_db, c.ratio};
	const float g = auto_gain_db(quiet, on, -1.0f);
	// ラウドネス -35 LUFS → 減衰 (6*3/4=4.5) → -39.5 LUFS から -18 LUFS へ → +21.5 dB
	CHECK(near(g, 21.5f, 0.05f));
	// ゲーム配信向けの -14 LUFS を目標にすると、+4 dB 多い
	CHECK(near(auto_gain_db(quiet, on, -1.0f, -14.0f), 25.5f, 0.05f));
	// コンプ後のピーク: -44 + 20/4 = -39 → 上限は -1+6+39 = 44 で、制限はかからない
	CHECK(g <= kGainMaxDb);

	// コンプなし: -35 LUFS をそのまま -18 LUFS へ → +17 dB
	const CompState off{false, 0.0f, 1.0f};
	CHECK(near(auto_gain_db(quiet, off, -1.0f), 17.0f, 0.05f));

	// 同じ入力は同じ結果
	CHECK(auto_gain_db(quiet, on, -1.0f) == auto_gain_db(quiet, on, -1.0f));

	// ピークが大きい人は、潰れすぎないようゲインを抑える
	Measurement peaky;
	peaky.speech_level_db = -30.0f;
	peaky.speech_lufs = -27.0f;
	peaky.peak_db = -3.0f;
	const float g2 = auto_gain_db(peaky, off, -1.0f);
	CHECK(near(g2, 8.0f, 0.05f)); // 目標は +9 dB だが、上限 -1 + 6 - (-3) = 8 dB に抑える
	CHECK(auto_limiter_ceiling_db() == -1.0f);

	// すでに大きい人はゲインを下げる（範囲内）
	Measurement loud;
	loud.speech_level_db = -8.0f;
	loud.speech_lufs = -5.0f;
	loud.peak_db = -2.0f;
	CHECK(auto_gain_db(loud, off, -1.0f) < 0.0f);
	CHECK(auto_gain_db(loud, off, -1.0f) >= kGainMinDb);
}

} // namespace

void test_auto_filters()
{
	// ゲート: ノイズ -65 dB, 声 -29 dB → -55 dB
	Measurement m;
	m.noise_floor_db = -65.0f;
	m.speech_level_db = -29.0f;
	CHECK(near(auto_gate_threshold_db(m), -55.0f, 1e-3f));
	// ノイズが声に近い場合は、声を切らないよう声の平均 - 12 dB まで
	m.noise_floor_db = -30.0f;
	CHECK(near(auto_gate_threshold_db(m), -41.0f, 1e-3f));
	CHECK(auto_highpass_hz() == 80.0f);

	// キーボード・マウスの音が測れているとき: ノイズ -65, 声 -29
	Measurement c;
	c.noise_floor_db = -65.0f;
	c.speech_level_db = -29.0f;
	c.keyboard_level_db = -50.0f; // 小さめ: +3 dB の -47 が、ノイズ基準の -55 より上
	c.mouse_level_db = -60.0f;
	CHECK(clicks_present(c));
	CHECK(near(auto_gate_threshold_db(c), -47.0f, 1e-3f));
	CHECK(!clicks_near_voice(c));
	c.keyboard_level_db = -33.0f; // 大きい: -30 だが、声の平均 - 8 dB = -37 まで
	CHECK(near(auto_gate_threshold_db(c), -37.0f, 1e-3f));
	CHECK(clicks_near_voice(c));
	// 測れていない（部屋のノイズと同程度）ときは、従来どおり
	c.keyboard_level_db = -64.0f;
	c.mouse_level_db = -90.0f;
	CHECK(!clicks_present(c));
	CHECK(near(auto_gate_threshold_db(c), -55.0f, 1e-3f));
	// クリック音がある環境では、ノイズ抑制は最大の強さ
	c.keyboard_level_db = -40.0f;
	CHECK(near(auto_denoise_percent(c), 100.0f, 1e-3f));

	// ノイズ抑制の強さ: 静かな部屋は弱め、うるさい部屋は強め
	Measurement quiet_room;
	quiet_room.speech_level_db = -29.0f;
	quiet_room.noise_floor_db = -80.0f;
	CHECK(near(auto_denoise_percent(quiet_room), 50.0f, 1e-3f));
	quiet_room.noise_floor_db = -65.0f;
	CHECK(near(auto_denoise_percent(quiet_room), 70.0f, 1e-3f));
	quiet_room.noise_floor_db = -40.0f;
	CHECK(near(auto_denoise_percent(quiet_room), 100.0f, 1e-3f));

	// EQ: こもっている声（300 Hz が 3 kHz より 8 dB 強い）→ こもりを 5.6 dB 抜き、明瞭さを 4.2 dB 足す
	Measurement muddy;
	muddy.band_low_db = -40.0f;
	muddy.band_mud_db = -30.0f;
	muddy.band_pres_db = -38.0f;
	const auto flat = vao::dsp::EqGains{};
	const auto g = auto_eq(muddy, flat);
	CHECK(near(g.mud_db, -5.6f, 1e-3f));
	CHECK(near(g.pres_db, 4.2f, 1e-3f));
	// さらにこもった声（300 Hz が 3 kHz より 11 dB 強い）→ こもりも明瞭さも上限の 6 dB

	Measurement dull;
	dull.band_low_db = -45.0f;
	dull.band_mud_db = -30.0f;
	dull.band_pres_db = -41.0f;
	CHECK(near(auto_eq(dull, flat).pres_db, 6.0f, 1e-3f));
	CHECK(near(auto_eq(dull, flat).mud_db, -6.0f, 1e-3f));
	// バランスがよい声は補正なし。プリセットの味付けだけが乗る
	Measurement good;
	good.band_low_db = -40.0f;
	good.band_mud_db = -35.0f;
	good.band_pres_db = -35.0f;
	const auto none = auto_eq(good, flat);
	CHECK(near(none.low_db, 0.0f, 1e-3f) && near(none.mud_db, 0.0f, 1e-3f) && near(none.pres_db, 0.0f, 1e-3f));
	const auto clear = vao::dsp::eq_preset(1);
	const auto withp = auto_eq(good, clear);
	CHECK(near(withp.pres_db, clear.pres_db, 1e-3f) && near(withp.air_db, clear.air_db, 1e-3f));
	// 上限 ±9 dB を超えない
	const auto big = auto_eq(muddy, vao::dsp::EqGains{-20.0f, -20.0f, 20.0f, 20.0f});
	CHECK(big.low_db >= -vao::dsp::kEqMaxDb && big.mud_db >= -vao::dsp::kEqMaxDb &&
	      big.pres_db <= vao::dsp::kEqMaxDb);

	// ディエッサー: サ行の平均 + 1 dB
	Measurement s;
	s.band_sib_db = -45.0f;
	CHECK(near(auto_deesser_threshold_db(s), -44.0f, 1e-3f));
	s.band_sib_db = -90.0f;
	CHECK(auto_deesser_threshold_db(s) >= -70.0f);
}

void test_input_gain()
{
	// 声が小さい人: 平均 -38 → -20 に合わせて +18 dB（ピーク -24 → -6 で、上限 -1 に収まる）
	Measurement quiet;
	quiet.noise_floor_db = -60.0f;
	quiet.speech_level_db = -38.0f;
	quiet.peak_db = -24.0f;
	CHECK(near(auto_input_gain_db(quiet), 18.0f, 1e-3f));

	// ピークが大きい人: 目標は +10 dB だが、ピークが -1 dB を超えない +5 dB に抑える
	Measurement peaky;
	peaky.speech_level_db = -30.0f;
	peaky.peak_db = -6.0f;
	CHECK(near(auto_input_gain_db(peaky), 5.0f, 1e-3f));

	// すでに大きい人: 下げる（下限 -12 dB まで）
	Measurement loud;
	loud.speech_level_db = -8.0f;
	loud.peak_db = -2.0f;
	CHECK(near(auto_input_gain_db(loud), -12.0f, 1e-3f));

	// 極端に小さい人: 上限 +40 dB
	Measurement tiny;
	tiny.speech_level_db = -60.0f;
	tiny.peak_db = -50.0f;
	CHECK(near(auto_input_gain_db(tiny), 40.0f, 1e-3f));

	// 大きさの値をずらす。音割れの印は変わらない
	Measurement shifted = shift_levels(quiet, 18.0f);
	CHECK(near(shifted.noise_floor_db, -42.0f, 1e-3f));
	CHECK(near(shifted.speech_level_db, -20.0f, 1e-3f));
	CHECK(near(shifted.peak_db, -6.0f, 1e-3f));
	quiet.speech_lufs = -35.0f;
	CHECK(near(shift_levels(quiet, 18.0f).speech_lufs, -17.0f, 1e-3f));
	Measurement clipped = quiet;
	clipped.clipped = true;
	CHECK(shift_levels(clipped, 5.0f).clipped);

	// ずらしても、声と雑音の差で決まるもの（ノイズ抑制の強さ・クリックの判定）は変わらない
	CHECK(near(auto_denoise_percent(shifted), auto_denoise_percent(quiet), 1e-3f));
	// ゲートのしきい値は、ずらした分だけ動く（雑音 -60・声 -38 → -50。ずらすと -32）
	CHECK(near(auto_gate_threshold_db(quiet), -50.0f, 1e-3f));
	CHECK(near(auto_gate_threshold_db(shifted), -32.0f, 1e-3f));

	// 声がとても小さいときは、マイク側の音量を上げるよう案内する
	Measurement low;
	low.speech_level_db = -45.0f;
	CHECK(input_too_quiet(low));
	low.speech_level_db = -30.0f;
	CHECK(!input_too_quiet(low));
}

void test_measured_bands_follow_voice()
{
	// 3 kHz の声: 明瞭さの帯域が強い → EQ はこもりを抜かず、高域を少し抑える
	Measurer m;
	m.prepare(48000.0f);
	m.start();
	run_session(m, 48000.0f, 0.001f, 0.05f, 0.4f, 1, 3000.0f);
	Measurement r;
	CHECK(m.compute(r));
	CHECK(r.band_pres_db > r.band_mud_db + 10.0f);
	CHECK(auto_eq(r, vao::dsp::EqGains{}).mud_db == 0.0f);
	CHECK(auto_eq(r, vao::dsp::EqGains{}).pres_db < 0.0f);
	// 7 kHz の声: サ行の帯域が強い
	Measurer m2;
	m2.prepare(48000.0f);
	m2.start();
	run_session(m2, 48000.0f, 0.001f, 0.05f, 0.4f, 1, 7000.0f);
	Measurement r2;
	CHECK(m2.compute(r2));
	CHECK(r2.band_sib_db > r2.band_mud_db + 10.0f);
}

int main()
{
	test_measurement();
	test_click_sampling();
	test_same_input_same_result();
	test_no_voice_fails();
	test_clipping_detected();
	test_restart_and_cancel();
	test_auto_settings();
	test_auto_filters();
	test_input_gain();
	test_measured_bands_follow_voice();
	return vao_test::finish();
}
