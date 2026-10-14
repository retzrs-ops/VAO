// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

// OBS・Qt に依存しないコンプレッサー（全チャンネル連動）。process() は確保・ロック・ログをしない。

#pragma once

#include <cstddef>

namespace vao::dsp {

class Compressor {
public:
	void prepare(float sample_rate); // 設定変更時に呼ぶ（音声スレッドでは呼ばない）

	// オフのときは音を変えない（効きは release の時間で滑らかに戻る）。
	void set_params(bool enabled, float threshold_db, float ratio, float attack_ms = 10.0f,
			float release_ms = 150.0f);

	void reset();

	void process(float *const *channels, std::size_t num_channels, std::size_t num_frames);

private:
	float sample_rate_ = 48000.0f;
	bool enabled_ = false;
	float threshold_db_ = -20.0f;
	float ratio_ = 3.0f;
	float attack_coef_ = 0.0f;
	float release_coef_ = 0.0f;
	float env_ = 0.0f;   // ピークの包絡（倍率）
	float gr_db_ = 0.0f; // 現在の減衰量（dB、0 以上）
};

} // namespace vao::dsp
