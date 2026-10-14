// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

// OBS・Qt に依存する物は持たない先読みリミッター（全チャンネル連動）。
// 約 2 ms 先読みするため、音が約 2 ms 遅れる（オフでも遅れは同じ）。
// process() は確保・ロック・ログをしない。

#pragma once

#include <cstddef>
#include <vector>

namespace vao::dsp {

class Limiter {
public:
	static constexpr std::size_t kMaxChannels = 8;

	void prepare(float sample_rate); // バッファを確保する。音声スレッドでは呼ばない

	void set_params(bool enabled, float ceiling_db);

	void reset();

	// 遅れるフレーム数
	std::size_t latency_frames() const { return lookahead_ > 0 ? lookahead_ - 1 : 0; }

	// num_channels が kMaxChannels を超える場合は何もしない（素通し）。
	void process(float *const *channels, std::size_t num_channels, std::size_t num_frames);

private:
	float sample_rate_ = 48000.0f;
	bool enabled_ = true;
	float ceiling_db_ = -1.0f;
	float ceiling_ = 0.8912509f;
	float release_k_ = 0.0f;

	std::size_t lookahead_ = 0;
	std::size_t pos_ = 0;
	float gain_prev_ = 1.0f;
	std::vector<float> delay_; // kMaxChannels * lookahead_
	std::vector<float> need_;  // 各フレームで必要な倍率の履歴
	std::vector<float> minw_;  // 窓内の最小値の履歴
};

} // namespace vao::dsp
