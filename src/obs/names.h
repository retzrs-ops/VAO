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
#define VAO_KEY_HPF_ON "hpf_on"
#define VAO_KEY_HPF_FREQ "hpf_freq_hz"
#define VAO_KEY_NR_ON "nr_on"
#define VAO_KEY_NR_STRENGTH "nr_strength_pct"
#define VAO_KEY_GATE_ON "gate_on"
#define VAO_KEY_GATE_THRESHOLD "gate_threshold_db"
#define VAO_KEY_EQ_ON "eq_on"
#define VAO_KEY_EQ_PRESET "eq_preset"
#define VAO_KEY_EQ_LOW "eq_low_db"
#define VAO_KEY_EQ_MUD "eq_mud_db"
#define VAO_KEY_EQ_PRES "eq_pres_db"
#define VAO_KEY_EQ_AIR "eq_air_db"
#define VAO_KEY_DEESS_ON "deess_on"
#define VAO_KEY_DEESS_THRESHOLD "deess_threshold_db"
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
#define VAO_TEXT_STATUS_KEYBOARD "StatusKeyboard"
#define VAO_TEXT_STATUS_MOUSE "StatusMouse"
#define VAO_TEXT_STATUS_CLICKS_LOUD "StatusClicksLoud"
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
#define VAO_TEXT_HPF "HighPass"
#define VAO_TEXT_HPF_FREQ "HpfFreq"
#define VAO_TEXT_NR "NoiseReduction"
#define VAO_TEXT_NR_STRENGTH "NrStrength"
#define VAO_TEXT_GATE "NoiseGate"
#define VAO_TEXT_GATE_THRESHOLD "GateThreshold"
#define VAO_TEXT_EQ "Eq"
#define VAO_TEXT_EQ_PRESET "EqPreset"
#define VAO_TEXT_EQ_LOW "EqLow"
#define VAO_TEXT_EQ_MUD "EqMud"
#define VAO_TEXT_EQ_PRES "EqPres"
#define VAO_TEXT_EQ_AIR "EqAir"
#define VAO_TEXT_PRESET_NATURAL "PresetNatural"
#define VAO_TEXT_PRESET_CLEAR "PresetClear"
#define VAO_TEXT_PRESET_WARM "PresetWarm"
#define VAO_TEXT_PRESET_RADIO "PresetRadio"
#define VAO_TEXT_PRESET_SOFT "PresetSoft"
#define VAO_TEXT_PRESET_CRISP "PresetCrisp"
#define VAO_TEXT_DEESS "Deesser"
#define VAO_TEXT_DEESS_THRESHOLD "DeessThreshold"
