// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

// OBS・Qt に依存しないゲイン処理。process() は確保・ロック・ログをしない。

#pragma once

#include <cstddef>

namespace vao::dsp {

constexpr float kMinGainDb = -120.0f; // これ以下は無音（ゲイン 0）
constexpr float kMaxGainDb = 60.0f;

// dB を倍率に変える。範囲外は kMinGainDb〜kMaxGainDb に丸める。kMinGainDb 以下は 0。
float db_to_linear(float db);

class Gain {
public:
	// 目標のゲイン。次の process() で、そのブロックの中で滑らかに近づける（プチ音対策）。
	void set_gain_db(float db);
	float gain_db() const { return db_; }

	// 現在値を目標へ即座にそろえる（滑らかにしない）。
	void reset();

	// planar（チャンネルごとの配列）の音をその場で書き換える。
	// channels[i] が nullptr のチャンネルは飛ばす。
	void process(float *const *channels, std::size_t num_channels, std::size_t num_frames);

private:
	float db_ = 0.0f;
	float target_ = 1.0f;
	float current_ = 1.0f;
};

} // namespace vao::dsp
