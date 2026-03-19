#pragma once

#include "Framework/UIRendererSection.h"

class Timer;

class TimeControls final : public UIRendererSection
{
  public:
	explicit TimeControls(Timer& timer) noexcept;
	~TimeControls() = default;

	TimeControls(const TimeControls&) = delete;
	TimeControls(TimeControls&&) = delete;
	TimeControls& operator=(const TimeControls&) = delete;
	TimeControls& operator=(TimeControls&&) = delete;

	UIRendererSectionId GetId() const noexcept override { return UIRendererSectionId::Time; }
	const char* GetTitle() const noexcept override { return "Time"; }

	void BuildUI() override;

  private:
	Timer& m_timer;
};
