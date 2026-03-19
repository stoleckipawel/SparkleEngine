#pragma once

#include "Framework/UIRendererSection.h"

class ViewMode final : public UIRendererSection
{
  public:
	ViewMode() noexcept = default;
	~ViewMode() = default;

	ViewMode(const ViewMode&) = delete;
	ViewMode(ViewMode&&) = delete;
	ViewMode& operator=(const ViewMode&) = delete;
	ViewMode& operator=(ViewMode&&) = delete;

	UIRendererSectionId GetId() const noexcept override { return UIRendererSectionId::ViewMode; }
	const char* GetTitle() const noexcept override { return "Viewport"; }

	void BuildUI() override;
};
