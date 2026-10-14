// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

// 名前の定義はここと buildspec.json の2か所にまとめる。
//  - buildspec.json : プラグイン名（VAO）、正式名「VAO – Voice Auto Optimizer」、作者、版
//  - このファイル   : フィルタ ID と、画面の文字を引く locale のキー名
//  - data/locale    : 画面に出る文字そのもの（「VAO Voice」など）
// 初回公開後は VAO_FILTER_ID と設定キーを変えない（保存された設定が消える）。

#pragma once

#define VAO_FILTER_ID "vao_voice_filter"

// locale（data/locale/*.ini）のキー
#define VAO_TEXT_FILTER_NAME "FilterName"
#define VAO_TEXT_MODULE_NAME "ModuleName"
#define VAO_TEXT_MODULE_DESC "ModuleDescription"
