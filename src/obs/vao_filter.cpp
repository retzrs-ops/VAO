// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

// OBS 音声フィルタ「VAO Voice」。OBS との境目だけを書く。音声処理の中身は dsp、測定と自動設定は calib。
// 処理の順番: コンプ → 出力ゲイン → リミッター（最後は必ずリミッター）。

#include "vao_filter.h"

#include <obs-module.h>
#include <plugin-support.h>

#include <atomic>
#include <cstdio>
#include <mutex>
#include <string>

#include "calib/auto_settings.h"
#include "calib/measurer.h"
#include "dsp/compressor.h"
#include "dsp/gain.h"
#include "dsp/limiter.h"
#include "names.h"

namespace {

using vao::calib::Measurement;
using vao::calib::Measurer;

constexpr std::size_t kMaxChannels = vao::dsp::Limiter::kMaxChannels;

struct vao_filter {
	obs_source_t *source = nullptr;

	// 設定（UI 側が書き、音声スレッドが読む）
	std::atomic<bool> gain_on{true};
	std::atomic<bool> comp_on{false};
	std::atomic<bool> lim_on{true};
	std::atomic<float> gain_db{0.0f};
	std::atomic<float> comp_threshold_db{-20.0f};
	std::atomic<float> comp_ratio{3.0f};
	std::atomic<float> lim_ceiling_db{-1.0f};

	// 音声スレッドだけが触る
	vao::dsp::Compressor comp;
	vao::dsp::Gain gain;
	vao::dsp::Limiter limiter;

	// 測定（feed は音声スレッド、ほかは UI・描画スレッド）
	Measurer meas;
	std::mutex mutex; // 下の3つを守る。音声スレッドでは使わない
	Measurement last;
	bool pending = false; // 測定が終わったら自動設定を実行する

	// 画面に出した状態（同じ表示を繰り返し更新しないため）
	std::atomic<int> shown_phase{-1};
	std::atomic<int> shown_remaining{-1};

	// フィルターに音が届いているかの確認用
	std::atomic<unsigned> audio_calls{0};
	float wait_sec = 0.0f; // 測定開始から音が来るのを待っている時間（描画スレッドだけが触る）
};

bool measuring(const vao_filter *f)
{
	switch (f->meas.phase()) {
	case Measurer::Phase::Starting:
	case Measurer::Phase::Silence:
	case Measurer::Phase::Speech:
	case Measurer::Phase::Loud:
		return true;
	default:
		return false;
	}
}

// ---- 画面の文字 ----

std::string format_int(const char *tmpl, int value)
{
	char buf[512];
	std::snprintf(buf, sizeof(buf), tmpl, value);
	return buf;
}

std::string format_int_str(const char *tmpl, int value, const char *str)
{
	char buf[1024];
	std::snprintf(buf, sizeof(buf), tmpl, value, str);
	return buf;
}

void set_status(vao_filter *f, const std::string &text)
{
	obs_data_t *settings = obs_source_get_settings(f->source);
	obs_data_set_string(settings, VAO_KEY_STATUS, text.c_str());
	obs_source_update(f->source, settings);
	obs_data_release(settings);
	obs_source_update_properties(f->source);
}

// ---- 自動設定を設定に書き込む ----

// 測定結果から3つのフィルターをすべて設定する
void apply_auto_settings(vao_filter *f)
{
	Measurement m;
	{
		std::lock_guard<std::mutex> lock(f->mutex);
		m = f->last;
	}

	obs_data_t *s = obs_source_get_settings(f->source);

	const float ceiling = vao::calib::auto_limiter_ceiling_db();
	obs_data_set_bool(s, VAO_KEY_LIM_ON, true);
	obs_data_set_double(s, VAO_KEY_LIM_CEILING, ceiling);

	const auto c = vao::calib::auto_comp(m);
	obs_data_set_bool(s, VAO_KEY_COMP_ON, true);
	obs_data_set_double(s, VAO_KEY_COMP_THRESHOLD, c.threshold_db);
	obs_data_set_double(s, VAO_KEY_COMP_RATIO, c.ratio);

	vao::calib::CompState cs;
	cs.enabled = true;
	cs.threshold_db = c.threshold_db;
	cs.ratio = c.ratio;
	obs_data_set_bool(s, VAO_KEY_GAIN_ON, true);
	obs_data_set_double(s, VAO_KEY_GAIN_DB, vao::calib::auto_gain_db(m, cs, ceiling));

	obs_source_update(f->source, s);
	obs_data_release(s);
}

// 測定が終わったあとの表示と、待たせていた自動設定の実行（描画スレッドから）
void finish_measurement(vao_filter *f)
{
	Measurement m;
	if (!f->meas.compute(m)) {
		{
			std::lock_guard<std::mutex> lock(f->mutex);
			f->pending = false;
		}
		set_status(f, obs_module_text(VAO_TEXT_STATUS_FAILED));
		return;
	}

	bool apply;
	{
		std::lock_guard<std::mutex> lock(f->mutex);
		f->last = m;
		apply = f->pending;
		f->pending = false;
	}
	if (apply)
		apply_auto_settings(f);

	std::string text = obs_module_text(VAO_TEXT_STATUS_DONE);
	if (m.clipped) {
		text += "\n";
		text += obs_module_text(VAO_TEXT_STATUS_CLIPPED);
	}
	set_status(f, text);
}

constexpr int kShownDisabled = 100; // shown_phase に入れる、Measurer::Phase と重ならない値
constexpr int kShownNoAudio = 101;

void vao_tick(void *data, float seconds)
{
	auto *f = static_cast<vao_filter *>(data);
	const Measurer::Phase p = f->meas.phase();
	int remaining = f->meas.remaining_sec();
	if (p == Measurer::Phase::Starting)
		remaining = Measurer::kSilenceSec;

	// 測定中なのに進まない理由を画面に出す（フィルターがオフ、または音が来ていない）
	if (measuring(f)) {
		f->wait_sec = p == Measurer::Phase::Starting ? f->wait_sec + seconds : 0.0f;
		int blocked = 0;
		if (!obs_source_enabled(f->source))
			blocked = kShownDisabled;
		else if (f->wait_sec >= 2.0f)
			blocked = kShownNoAudio;
		if (blocked != 0) {
			if (f->shown_phase.load() != blocked) {
				f->shown_phase = blocked;
				set_status(f, obs_module_text(blocked == kShownDisabled ? VAO_TEXT_STATUS_DISABLED
											: VAO_TEXT_STATUS_NO_AUDIO));
			}
			return;
		}
	}

	if (static_cast<int>(p) == f->shown_phase.load() && remaining == f->shown_remaining.load())
		return;
	f->shown_phase = static_cast<int>(p);
	f->shown_remaining = remaining;

	switch (p) {
	case Measurer::Phase::Starting:
	case Measurer::Phase::Silence:
		set_status(f, format_int(obs_module_text(VAO_TEXT_STATUS_SILENCE), remaining));
		break;
	case Measurer::Phase::Speech:
		set_status(f, format_int_str(obs_module_text(VAO_TEXT_STATUS_SPEECH), remaining,
					     obs_module_text(VAO_TEXT_PHRASE)));
		break;
	case Measurer::Phase::Loud:
		set_status(f, format_int(obs_module_text(VAO_TEXT_STATUS_LOUD), remaining));
		break;
	case Measurer::Phase::Done:
		finish_measurement(f);
		f->shown_phase = static_cast<int>(f->meas.phase()); // 失敗なら Failed になっている
		break;
	default:
		break;
	}
}

// ---- ボタン ----

// 「すべて自動設定」: 毎回測り直し、測定が終わったら設定する。
bool cb_auto_all(obs_properties_t *, obs_property_t *, void *data)
{
	auto *f = static_cast<vao_filter *>(data);
	if (measuring(f))
		return false;
	{
		std::lock_guard<std::mutex> lock(f->mutex);
		f->pending = true;
	}
	f->shown_phase = -1;
	f->meas.start();
	return false;
}

// ---- フィルタ本体 ----

const char *vao_get_name(void *)
{
	return obs_module_text(VAO_TEXT_FILTER_NAME);
}

void vao_update(void *data, obs_data_t *s)
{
	auto *f = static_cast<vao_filter *>(data);
	f->gain_on = obs_data_get_bool(s, VAO_KEY_GAIN_ON);
	f->gain_db = static_cast<float>(obs_data_get_double(s, VAO_KEY_GAIN_DB));
	f->comp_on = obs_data_get_bool(s, VAO_KEY_COMP_ON);
	f->comp_threshold_db = static_cast<float>(obs_data_get_double(s, VAO_KEY_COMP_THRESHOLD));
	f->comp_ratio = static_cast<float>(obs_data_get_double(s, VAO_KEY_COMP_RATIO));
	f->lim_on = obs_data_get_bool(s, VAO_KEY_LIM_ON);
	f->lim_ceiling_db = static_cast<float>(obs_data_get_double(s, VAO_KEY_LIM_CEILING));
}

void vao_defaults(obs_data_t *s)
{
	obs_data_set_default_string(s, VAO_KEY_STATUS, "");
	obs_data_set_default_bool(s, VAO_KEY_GAIN_ON, true);
	obs_data_set_default_double(s, VAO_KEY_GAIN_DB, 0.0);
	obs_data_set_default_bool(s, VAO_KEY_COMP_ON, false);
	obs_data_set_default_double(s, VAO_KEY_COMP_THRESHOLD, -20.0);
	obs_data_set_default_double(s, VAO_KEY_COMP_RATIO, 3.0);
	obs_data_set_default_bool(s, VAO_KEY_LIM_ON, true);
	obs_data_set_default_double(s, VAO_KEY_LIM_CEILING, -1.0);
}

void *vao_create(obs_data_t *settings, obs_source_t *source)
{
	auto *f = new vao_filter();
	f->source = source;

	float rate = 48000.0f;
	if (audio_t *audio = obs_get_audio())
		rate = static_cast<float>(audio_output_get_sample_rate(audio));
	f->comp.prepare(rate);
	f->limiter.prepare(rate);
	f->meas.prepare(rate);

	obs_data_set_string(settings, VAO_KEY_STATUS, obs_module_text(VAO_TEXT_STATUS_IDLE));
	vao_update(f, settings);

	obs_add_tick_callback(vao_tick, f);
	return f;
}

void vao_destroy(void *data)
{
	auto *f = static_cast<vao_filter *>(data);
	obs_remove_tick_callback(vao_tick, f);
	delete f;
}

obs_properties_t *vao_properties(void *data)
{
	obs_properties_t *props = obs_properties_create_param(data, nullptr);

	obs_properties_add_text(props, VAO_KEY_STATUS, obs_module_text(VAO_TEXT_STATUS_LABEL), OBS_TEXT_INFO);
	obs_properties_add_button(props, VAO_BTN_AUTO_ALL, obs_module_text(VAO_TEXT_AUTO_ALL), cb_auto_all);

	// コンプレッサー（グループ名のチェックボックスがオンオフ）
	obs_properties_t *comp = obs_properties_create_param(data, nullptr);
	obs_property_t *p = obs_properties_add_float_slider(comp, VAO_KEY_COMP_THRESHOLD,
							    obs_module_text(VAO_TEXT_COMP_THRESHOLD), -60.0, 0.0, 0.5);
	obs_property_float_set_suffix(p, " dB");
	obs_properties_add_float_slider(comp, VAO_KEY_COMP_RATIO, obs_module_text(VAO_TEXT_COMP_RATIO), 1.0, 10.0, 0.1);
	obs_properties_add_group(props, VAO_KEY_COMP_ON, obs_module_text(VAO_TEXT_COMP), OBS_GROUP_CHECKABLE, comp);

	// 出力ゲイン
	obs_properties_t *gain = obs_properties_create_param(data, nullptr);
	p = obs_properties_add_float_slider(gain, VAO_KEY_GAIN_DB, obs_module_text(VAO_TEXT_GAIN_DB), -20.0, 40.0, 0.5);
	obs_property_float_set_suffix(p, " dB");
	obs_properties_add_group(props, VAO_KEY_GAIN_ON, obs_module_text(VAO_TEXT_GAIN), OBS_GROUP_CHECKABLE, gain);

	// リミッター
	obs_properties_t *lim = obs_properties_create_param(data, nullptr);
	p = obs_properties_add_float_slider(lim, VAO_KEY_LIM_CEILING, obs_module_text(VAO_TEXT_LIM_CEILING), -12.0, 0.0, 0.5);
	obs_property_float_set_suffix(p, " dB");
	obs_properties_add_group(props, VAO_KEY_LIM_ON, obs_module_text(VAO_TEXT_LIMITER), OBS_GROUP_CHECKABLE, lim);

	return props;
}

// 音声処理スレッド: 確保・ロック・ログ・ファイル操作をしない。
obs_audio_data *vao_filter_audio(void *data, obs_audio_data *audio)
{
	auto *f = static_cast<vao_filter *>(data);

	float *planes[kMaxChannels];
	std::size_t nch = 0;
	for (std::size_t i = 0; i < kMaxChannels; ++i) {
		if (audio->data[i] == nullptr)
			break;
		planes[nch++] = reinterpret_cast<float *>(audio->data[i]);
	}
	const std::size_t frames = audio->frames;
	f->audio_calls.fetch_add(1, std::memory_order_relaxed);
	if (nch == 0 || frames == 0)
		return audio;

	f->meas.feed(planes, nch, frames); // 測るのは加工する前の音

	f->comp.set_params(f->comp_on.load(), f->comp_threshold_db.load(), f->comp_ratio.load());
	f->comp.process(planes, nch, frames);

	f->gain.set_gain_db(f->gain_on.load() ? f->gain_db.load() : 0.0f);
	f->gain.process(planes, nch, frames);

	f->limiter.set_params(f->lim_on.load(), f->lim_ceiling_db.load());
	f->limiter.process(planes, nch, frames);

	return audio;
}

obs_source_info make_filter_info()
{
	obs_source_info info = {};
	info.id = VAO_FILTER_ID;
	info.type = OBS_SOURCE_TYPE_FILTER;
	info.output_flags = OBS_SOURCE_AUDIO;
	info.get_name = vao_get_name;
	info.create = vao_create;
	info.destroy = vao_destroy;
	info.update = vao_update;
	info.get_defaults = vao_defaults;
	info.get_properties = vao_properties;
	info.filter_audio = vao_filter_audio;
	return info;
}

} // namespace

void vao_register_filter()
{
	static obs_source_info info = make_filter_info();
	obs_register_source(&info);
}
