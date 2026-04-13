#pragma once

#include "Frame/RenderViewContext.h"

class D3D12SwapChain;
class RenderCamera;
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
	    const D3D12_VIEWPORT& viewport,
	    const D3D12_RECT& scissorRect) const noexcept;

	RenderViewContext BuildMainView(
	    const RenderCamera& renderCamera,
	    const PerViewLightingConstantBufferData& lightingData,
	    const D3D12SwapChain& swapChain) const noexcept;

  private:
	static PerViewConstantBufferData BuildPerViewData(
	    const PerViewCameraConstantBufferData& cameraData,
	    const PerViewLightingConstantBufferData& lightingData) noexcept;
};
