// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

#include <obs-module.h>
#include <plugin-support.h>

#include "names.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

MODULE_EXPORT const char *obs_module_name(void)
{
	return obs_module_text(VAO_TEXT_MODULE_NAME);
}

MODULE_EXPORT const char *obs_module_description(void)
{
	return obs_module_text(VAO_TEXT_MODULE_DESC);
}

namespace {

struct vao_filter {
	obs_source_t *context;
};

const char *vao_filter_get_name(void *)
{
	return obs_module_text(VAO_TEXT_FILTER_NAME);
}

void *vao_filter_create(obs_data_t *, obs_source_t *source)
{
	auto *f = static_cast<vao_filter *>(bzalloc(sizeof(vao_filter)));
	f->context = source;
	return f;
}

void vao_filter_destroy(void *data)
{
	bfree(data);
}

// いまは音をそのまま通すだけ。音声処理スレッドなので確保・ロック・ログは禁止。
obs_audio_data *vao_filter_audio(void *, obs_audio_data *audio)
{
	return audio;
}

obs_source_info make_filter_info()
{
	obs_source_info info = {};
	info.id = VAO_FILTER_ID;
	info.type = OBS_SOURCE_TYPE_FILTER;
	info.output_flags = OBS_SOURCE_AUDIO;
	info.get_name = vao_filter_get_name;
	info.create = vao_filter_create;
	info.destroy = vao_filter_destroy;
	info.filter_audio = vao_filter_audio;
	return info;
}

} // namespace

bool obs_module_load(void)
{
	static obs_source_info info = make_filter_info();
	obs_register_source(&info);
	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	obs_log(LOG_INFO, "plugin unloaded");
}
