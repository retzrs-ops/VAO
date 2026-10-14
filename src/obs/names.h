// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

// 名前の定義はここと buildspec.json の2か所にまとめる。
//  - buildspec.json : プラグイン名（VAO）、正式名「VAO – Voice Auto Optimizer」、作者、版
//  - このファイル   : フィルタ ID、設定キー、画面の文字を引く locale のキー名
//  - data/locale    : 画面に出る文字そのもの（「VAO Voice」など）
// 初回公開後は VAO_FILTER_ID と設定キーを変えない（保存された設定が消える）。増やすのは可。

#pragma once

#define VAO_FILTER_ID "vao_voice_filter"

// 設定キー
#define VAO_KEY_STATUS "status"
#define VAO_KEY_GAIN_ON "gain_on"
#define VAO_KEY_GAIN_DB "gain_db"
#define VAO_KEY_COMP_ON "comp_on"
#define VAO_KEY_COMP_THRESHOLD "comp_threshold_db"
#define VAO_KEY_COMP_RATIO "comp_ratio"
#define VAO_KEY_LIM_ON "lim_on"
#define VAO_KEY_LIM_CEILING "lim_ceiling_db"

// ボタンの名前（設定としては保存されない）
#define VAO_BTN_AUTO_ALL "btn_auto_all"

// locale（data/locale/*.ini）のキー
#define VAO_TEXT_FILTER_NAME "FilterName"
#define VAO_TEXT_MODULE_NAME "ModuleName"
#define VAO_TEXT_MODULE_DESC "ModuleDescription"
#define VAO_TEXT_STATUS_LABEL "Status"
#define VAO_TEXT_STATUS_IDLE "StatusIdle"
#define VAO_TEXT_STATUS_SILENCE "StatusSilence"
#define VAO_TEXT_STATUS_SPEECH "StatusSpeech"
#define VAO_TEXT_STATUS_LOUD "StatusLoud"
#define VAO_TEXT_STATUS_DONE "StatusDone"
#define VAO_TEXT_STATUS_FAILED "StatusFailed"
#define VAO_TEXT_STATUS_CLIPPED "StatusClipped"
#define VAO_TEXT_STATUS_DISABLED "StatusDisabled"
#define VAO_TEXT_STATUS_NO_AUDIO "StatusNoAudio"
#define VAO_TEXT_PHRASE "Phrase"
#define VAO_TEXT_AUTO_ALL "AutoAll"
#define VAO_TEXT_COMP "Compressor"
#define VAO_TEXT_COMP_THRESHOLD "CompThreshold"
#define VAO_TEXT_COMP_RATIO "CompRatio"
#define VAO_TEXT_GAIN "OutputGain"
#define VAO_TEXT_GAIN_DB "GainDb"
#define VAO_TEXT_LIMITER "Limiter"
#define VAO_TEXT_LIM_CEILING "LimCeiling"
