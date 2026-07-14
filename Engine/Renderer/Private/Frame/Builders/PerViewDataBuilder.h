#pragma once

#include "Frame/Core/RenderViewData.h"

class PerViewDataBuilder final
{
  public:
	PerViewDataBuilder() noexcept = default;
	~PerViewDataBuilder() noexcept = default;

	PerViewDataBuilder(const PerViewDataBuilder&) = delete;
	PerViewDataBuilder& operator=(const PerViewDataBuilder&) = delete;
	PerViewDataBuilder(PerViewDataBuilder&&) = delete;
	PerViewDataBuilder& operator=(PerViewDataBuilder&&) = delete;

	RenderViewData BuildView(
	    const PerViewCameraConstantBufferData& cameraData,
	    const RhiViewport& viewport,
	    const RhiRect& scissorRect) const noexcept;

  private:
	static PerViewConstantBufferData BuildPerViewData(const PerViewCameraConstantBufferData& cameraData) noexcept;
};
