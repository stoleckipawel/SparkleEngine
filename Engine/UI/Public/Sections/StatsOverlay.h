// ============================================================================
// StatsOverlay.h
// ----------------------------------------------------------------------------
// UI section displaying performance statistics (FPS, frame time, frame index).
//
#pragma once

#include "Framework/UIRendererSection.h"

class Timer;

class StatsOverlay final : public UIRendererSection
{
  public:
	explicit StatsOverlay(Timer& timer) noexcept;
	~StatsOverlay() = default;

	StatsOverlay(const StatsOverlay&) = delete;
	StatsOverlay(StatsOverlay&&) = delete;
	StatsOverlay& operator=(const StatsOverlay&) = delete;
	StatsOverlay& operator=(StatsOverlay&&) = delete;


	UIRendererSectionId GetId() const noexcept override { return UIRendererSectionId::Stats; }
	const char* GetTitle() const noexcept override { return "Stats"; }

	// Builds the overlay contents. Call once per-frame while an ImGui frame is active.
	void BuildUI() override;

  private:
	Timer& m_timer;
};
