// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

// ノイズゲート（声がないときの「サーッ」を静かにする）。全チャンネル連動。
// 判定は短い時間の二乗平均(dB)で行う。しきい値は測定した部屋のノイズ(二乗平均)に合わせる。

#pragma once

#include <cstddef>

namespace vao::dsp {

class NoiseGate {
public:
	static constexpr float kClosedGainDb = -50.0f; // 閉じたときの減衰量

	void prepare(float sample_rate); // 音声スレッドでは呼ばない
	void set_params(bool enabled, float threshold_db);
	void reset();
	void process(float *const *channels, std::size_t num_channels, std::size_t num_frames);

private:
	float sample_rate_ = 48000.0f;
	bool enabled_ = false;
	float threshold_db_ = -50.0f;
	float level_k_ = 0.0f;   // レベル検出の平滑化
	float attack_k_ = 0.0f;  // 開くとき（速い）
	float release_k_ = 0.0f; // 閉じるとき（ゆっくり）
	std::size_t hold_samples_ = 0;

	float power_ = 0.0f;
	bool open_ = true;
	std::size_t hold_ = 0;
	float gain_ = 1.0f;
};

} // namespace vao::dsp
