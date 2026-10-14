// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

#include <obs-module.h>
#include <plugin-support.h>

#include "names.h"
#include "vao_filter.h"

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

bool obs_module_load(void)
{
	vao_register_filter();
	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	obs_log(LOG_INFO, "plugin unloaded");
}
