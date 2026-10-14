// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

#include "measurer.h"

#include <algorithm>
#include <cmath>

#include "dsp/deesser.h"

namespace vao::calib {

namespace {

constexpr int kBlocksPerSec = 20; // kBlockSec = 0.05

float to_db_power(double p)
{
	return 10.0f * std::log10(static_cast<float>(p) + 1e-12f);
}

} // namespace

int Measurer::blocks_in(Phase p) const
{
	switch (p) {
	case Phase::Silence:
		return kSilenceSec * kBlocksPerSec;
	case Phase::Keyboard:
		return kKeyboardSec * kBlocksPerSec;
	case Phase::Mouse:
		return kMouseSec * kBlocksPerSec;
	case Phase::Speech:
		return kSpeechSec * kBlocksPerSec;
	case Phase::Loud:
		return kLoudSec * kBlocksPerSec;
	default:
		return 0;
	}
}

void Measurer::prepare(float sample_rate)
{
	sample_rate_ = sample_rate > 0.0f ? sample_rate : 48000.0f;
	block_len_ = std::max<std::size_t>(1, static_cast<std::size_t>(std::lround(sample_rate_ * kBlockSec)));
	const std::size_t total =
		static_cast<std::size_t>(blocks_in(Phase::Silence) + blocks_in(Phase::Keyboard) +
					 blocks_in(Phase::Mouse) + blocks_in(Phase::Speech) + blocks_in(Phase::Loud));
	power_.assign(total, 0.0f);
	peak_.assign(total, 0.0f);
	band_power_.assign(total * kBands, 0.0f);
	det_k_ = 1.0 - std::exp(-1.0 / (0.010 * static_cast<double>(sample_rate_)));

	// 帯域: 低域 100 Hz / こもり 300 Hz / 明瞭さ 3 kHz / サ行（ディエッサーと同じ）
	const double fs = sample_rate_;
	band_filter_[0].set_coefs(vao::dsp::biquad_bandpass(fs, 100.0, 0.7));
	band_filter_[1].set_coefs(vao::dsp::biquad_bandpass(fs, 300.0, 1.0));
	band_filter_[2].set_coefs(vao::dsp::biquad_bandpass(fs, 3000.0, 1.0));
	band_filter_[3].set_coefs(vao::dsp::biquad_bandpass(fs, vao::dsp::kDeesserFreqHz, vao::dsp::kDeesserQ));
	phase_.store(Phase::Idle, std::memory_order_release);
}

void Measurer::start()
{
	phase_.store(Phase::Starting, std::memory_order_release);
}

void Measurer::cancel()
{
	phase_.store(Phase::Idle, std::memory_order_release);
}

int Measurer::remaining_sec() const
{
	const Phase p = phase();
	const int total = blocks_in(p);
	if (total == 0)
		return 0;
	const int left = total - block_in_phase_.load(std::memory_order_acquire);
	return std::max(0, (left + kBlocksPerSec - 1) / kBlocksPerSec);
}

bool Measurer::advance(Phase from, Phase to)
{
	Phase expected = from;
	return phase_.compare_exchange_strong(expected, to, std::memory_order_acq_rel);
}

void Measurer::feed(const float *const *channels, std::size_t num_channels, std::size_t num_frames)
{
	Phase p = phase_.load(std::memory_order_acquire);
	if (p == Phase::Idle || p == Phase::Done || p == Phase::Failed)
		return;
	if (channels == nullptr || num_channels == 0 || power_.empty())
		return;

	if (p == Phase::Starting) {
		frames_in_block_ = 0;
		sum_sq_ = 0.0;
		for (int b = 0; b < kBands; ++b) {
			band_sq_[b] = 0.0;
			band_filter_[b].reset();
		}
		block_peak_ = 0.0f;
		clip_run_ = 0;
		clipped_ = false;
		det_power_ = 0.0;
		click_max_[0] = click_max_[1] = 0.0;
		stored_ = 0;
		block_in_phase_.store(0, std::memory_order_release);
		if (!advance(Phase::Starting, Phase::Silence))
			return;
		p = Phase::Silence;
	}

	for (std::size_t i = 0; i < num_frames; ++i) {
		double sq = 0.0;
		double mono = 0.0;
		float frame_peak = 0.0f;
		std::size_t used = 0;
		for (std::size_t c = 0; c < num_channels; ++c) {
			if (channels[c] == nullptr)
				continue;
			const float x = channels[c][i];
			sq += static_cast<double>(x) * static_cast<double>(x);
			mono += static_cast<double>(x);
			frame_peak = std::max(frame_peak, std::fabs(x));
			++used;
		}
		if (used == 0)
			continue;
		sum_sq_ += sq / static_cast<double>(used);
		const float mono_x = static_cast<float>(mono / static_cast<double>(used));
		for (int b = 0; b < kBands; ++b) {
			const double y = band_filter_[b].process(mono_x);
			band_sq_[b] += y * y;
		}
		block_peak_ = std::max(block_peak_, frame_peak);

		// キーボード・マウスの音の強さ: ノイズゲートと同じ「10 ms の二乗平均（チャンネルの最大）」で測る
		{
			double loudest = 0.0;
			for (std::size_t c = 0; c < num_channels; ++c) {
				if (channels[c] != nullptr) {
					const double v = static_cast<double>(channels[c][i]);
					loudest = std::max(loudest, v * v);
				}
			}
			det_power_ += (loudest - det_power_) * det_k_;
			if (p == Phase::Keyboard)
				click_max_[0] = std::max(click_max_[0], det_power_);
			else if (p == Phase::Mouse)
				click_max_[1] = std::max(click_max_[1], det_power_);
		}

		if (frame_peak >= 0.999f) {
			if (++clip_run_ >= 3)
				clipped_ = true;
		} else {
			clip_run_ = 0;
		}

		if (++frames_in_block_ < block_len_)
			continue;

		// ブロック完了
		if (stored_ < power_.size()) {
			power_[stored_] = static_cast<float>(sum_sq_ / static_cast<double>(block_len_));
			peak_[stored_] = block_peak_;
			for (int b = 0; b < kBands; ++b)
				band_power_[stored_ * kBands + static_cast<std::size_t>(b)] =
					static_cast<float>(band_sq_[b] / static_cast<double>(block_len_));
			++stored_;
		}
		frames_in_block_ = 0;
		sum_sq_ = 0.0;
		for (int b = 0; b < kBands; ++b)
			band_sq_[b] = 0.0;
		block_peak_ = 0.0f;

		const int done = block_in_phase_.load(std::memory_order_relaxed) + 1;
		if (done < blocks_in(p)) {
			block_in_phase_.store(done, std::memory_order_release);
			continue;
		}

		// 段階完了
		Phase next = Phase::Done;
		if (p == Phase::Silence)
			next = Phase::Keyboard;
		else if (p == Phase::Keyboard)
			next = Phase::Mouse;
		else if (p == Phase::Mouse)
			next = Phase::Speech;
		else if (p == Phase::Speech)
			next = Phase::Loud;
		block_in_phase_.store(0, std::memory_order_release);
		if (!advance(p, next))
			return; // 途中で start()/cancel() された
		p = next;
		if (p == Phase::Done)
			return;
	}
}

bool Measurer::compute(Measurement &out)
{
	if (phase() != Phase::Done)
		return false;

	const std::size_t n_sil = static_cast<std::size_t>(blocks_in(Phase::Silence));
	const std::size_t n_kbd = static_cast<std::size_t>(blocks_in(Phase::Keyboard));
	const std::size_t n_mou = static_cast<std::size_t>(blocks_in(Phase::Mouse));
	const std::size_t n_spe = static_cast<std::size_t>(blocks_in(Phase::Speech));
	const std::size_t n_lou = static_cast<std::size_t>(blocks_in(Phase::Loud));
	const std::size_t speech_start = n_sil + n_kbd + n_mou;
	if (stored_ < speech_start + n_spe + n_lou) {
		phase_.store(Phase::Failed, std::memory_order_release);
		return false;
	}

	double noise = 0.0;
	const std::size_t skip = static_cast<std::size_t>(kSilenceSkipBlocks);
	for (std::size_t i = skip; i < n_sil; ++i)
		noise += power_[i];
	noise /= static_cast<double>(n_sil - skip);
	const float noise_db = to_db_power(noise);

	// 部屋が極端に静かでも、しきい値は -65 dB より下げない
	const float active_db = std::max(noise_db, -65.0f) + static_cast<float>(kActiveMarginDb);
	double active_sum = 0.0;
	double band_sum[kBands] = {};
	int active = 0;
	float peak = 0.0f;
	for (std::size_t i = speech_start; i < speech_start + n_spe; ++i) {
		peak = std::max(peak, peak_[i]);
		if (to_db_power(power_[i]) > active_db) {
			active_sum += power_[i];
			for (int b = 0; b < kBands; ++b)
				band_sum[b] += band_power_[i * kBands + static_cast<std::size_t>(b)];
			++active;
		}
	}
	for (std::size_t i = speech_start + n_spe; i < speech_start + n_spe + n_lou; ++i)
		peak = std::max(peak, peak_[i]);

	if (active < kMinActiveBlocks) {
		phase_.store(Phase::Failed, std::memory_order_release);
		return false;
	}

	out.noise_floor_db = noise_db;
	out.keyboard_level_db = to_db_power(click_max_[0]);
	out.mouse_level_db = to_db_power(click_max_[1]);
	out.speech_level_db = to_db_power(active_sum / static_cast<double>(active));
	out.peak_db = 20.0f * std::log10(peak + 1e-9f);
	out.clipped = clipped_;
	const double n = static_cast<double>(active);
	out.band_low_db = to_db_power(band_sum[0] / n);
	out.band_mud_db = to_db_power(band_sum[1] / n);
	out.band_pres_db = to_db_power(band_sum[2] / n);
	out.band_sib_db = to_db_power(band_sum[3] / n);
	return true;
}

} // namespace vao::calib
