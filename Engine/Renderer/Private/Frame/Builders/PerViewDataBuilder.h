#pragma once

#include "Frame/RenderViewContext.h"

class RenderCamera;
class RenderHardwareInterface;
struct PerViewLightingConstantBufferData;

class PerViewDataBuilder final
{
  public:
	PerViewDataBuilder() noexcept = default;
	~PerViewDataBuilder() noexcept = default;

	PerViewDataBuilder(const PerViewDataBuilder&) = delete;
	PerViewDataBuilder& operator=(const PerViewDataBuilder&) = delete;
	PerViewDataBuilder(PerViewDataBuilder&&) = delete;
	PerViewDataBuilder& operator=(PerViewDataBuilder&&) = delete;

	RenderViewContext BuildView(
	    const PerViewCameraConstantBufferData& cameraData,
	    const PerViewLightingConstantBufferData& lightingData,
	    const RhiViewport& viewport,
	    const RhiRect& scissorRect) const noexcept;

	RenderViewContext BuildMainView(
	    const RenderCamera& renderCamera,
	    const PerViewLightingConstantBufferData& lightingData,
	    const RenderHardwareInterface& renderHardwareInterface) const noexcept;

  private:
	static PerViewConstantBufferData BuildPerViewData(
	    const PerViewCameraConstantBufferData& cameraData,
	    const PerViewLightingConstantBufferData& lightingData) noexcept;
};
