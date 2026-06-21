#pragma once

#include "Frame/Core/RenderViewData.h"

class RenderCamera;
class RenderHardwareInterface;

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

	RenderViewData BuildMainView(
	    const RenderCamera& renderCamera,
	    const RenderHardwareInterface& renderHardwareInterface) const noexcept;

  private:
	static PerViewConstantBufferData BuildPerViewData(const PerViewCameraConstantBufferData& cameraData) noexcept;
};
