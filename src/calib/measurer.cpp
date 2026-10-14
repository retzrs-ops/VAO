// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Retzn (VAO - Voice Auto Optimizer)

#include "measurer.h"

#include <algorithm>
#include <cmath>

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
	const std::size_t total = static_cast<std::size_t>(
		blocks_in(Phase::Silence) + blocks_in(Phase::Speech) + blocks_in(Phase::Loud));
	power_.assign(total, 0.0f);
	peak_.assign(total, 0.0f);
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
		block_peak_ = 0.0f;
		clip_run_ = 0;
		clipped_ = false;
		stored_ = 0;
		block_in_phase_.store(0, std::memory_order_release);
		if (!advance(Phase::Starting, Phase::Silence))
			return;
		p = Phase::Silence;
	}

	for (std::size_t i = 0; i < num_frames; ++i) {
		double sq = 0.0;
		float frame_peak = 0.0f;
		std::size_t used = 0;
		for (std::size_t c = 0; c < num_channels; ++c) {
			if (channels[c] == nullptr)
				continue;
			const float x = channels[c][i];
			sq += static_cast<double>(x) * static_cast<double>(x);
			frame_peak = std::max(frame_peak, std::fabs(x));
			++used;
		}
		if (used == 0)
			continue;
		sum_sq_ += sq / static_cast<double>(used);
		block_peak_ = std::max(block_peak_, frame_peak);

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
			++stored_;
		}
		frames_in_block_ = 0;
		sum_sq_ = 0.0;
		block_peak_ = 0.0f;

		const int done = block_in_phase_.load(std::memory_order_relaxed) + 1;
		if (done < blocks_in(p)) {
			block_in_phase_.store(done, std::memory_order_release);
			continue;
		}

		// 段階完了
		Phase next = Phase::Done;
		if (p == Phase::Silence)
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
	const std::size_t n_spe = static_cast<std::size_t>(blocks_in(Phase::Speech));
	const std::size_t n_lou = static_cast<std::size_t>(blocks_in(Phase::Loud));
	if (stored_ < n_sil + n_spe + n_lou) {
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
	int active = 0;
	float peak = 0.0f;
	for (std::size_t i = n_sil; i < n_sil + n_spe; ++i) {
		peak = std::max(peak, peak_[i]);
		if (to_db_power(power_[i]) > active_db) {
			active_sum += power_[i];
			++active;
		}
	}
	for (std::size_t i = n_sil + n_spe; i < n_sil + n_spe + n_lou; ++i)
		peak = std::max(peak, peak_[i]);

	if (active < kMinActiveBlocks) {
		phase_.store(Phase::Failed, std::memory_order_release);
		return false;
	}

	out.noise_floor_db = noise_db;
	out.speech_level_db = to_db_power(active_sum / static_cast<double>(active));
	out.peak_db = 20.0f * std::log10(peak + 1e-9f);
	out.clipped = clipped_;
	return true;
}

} // namespace vao::calib
