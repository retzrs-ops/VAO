// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

// AI ノイズ抑制（RNNoise を内蔵）。OBS・Qt に依存しない。
// 48 kHz のときだけ動く（RNNoise の仕様）。それ以外のサンプルレートでは何もしない（遅れもなし）。
// 10 ms ずつまとめて処理するため、音が 10 ms 遅れる（オフでも遅れは同じ）。
// process() は確保・ロック・ログをしない。確保は prepare() だけ。
//
// RNNoise: Copyright (c) 2017 Mozilla, 2007-2017 Jean-Marc Valin, Xiph.Org Foundation ほか（BSD-3-Clause）。
// ライセンス文は licenses/RNNoise-COPYING.txt。

#pragma once

#include <array>
#include <cstddef>

struct DenoiseState;

namespace vao::dsp {

class Denoiser {
public:
	static constexpr std::size_t kFrame = 480; // 48 kHz で 10 ms
	static constexpr std::size_t kMaxChannels = 8;

	Denoiser();
	~Denoiser();
	Denoiser(const Denoiser &) = delete;
	Denoiser &operator=(const Denoiser &) = delete;

	// 音声スレッドでは呼ばない。48 kHz 以外なら false（以後は素通し）。
	bool prepare(float sample_rate);
	bool supported() const { return supported_; }

	// strength: 0〜1（1 でノイズ抑制を 100% かける。0 は原音のまま）
	void set_params(bool enabled, float strength);
	void reset();

	std::size_t latency_frames() const { return supported_ ? kFrame : 0; }

	// num_channels が kMaxChannels を超える場合は何もしない（素通し）。
	void process(float *const *channels, std::size_t num_channels, std::size_t num_frames);

private:
	static constexpr std::size_t kRing = 512; // kFrame + 余裕

	struct Channel {
		DenoiseState *state = nullptr;
		float in[kFrame] = {};
		float out[kRing] = {};
		std::size_t in_fill = 0;
		std::size_t out_read = 0;
		std::size_t out_count = 0;
	};

	void run_frame(Channel &c);
	void reset_channel(Channel &c);

	bool supported_ = false;
	bool enabled_ = false;
	float strength_ = 1.0f;
	std::array<Channel, kMaxChannels> ch_;
};

} // namespace vao::dsp
