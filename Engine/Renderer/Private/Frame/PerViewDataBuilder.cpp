#include "PCH.h"

#include "PerViewDataBuilder.h"

#include "D3D12SwapChain.h"
#include "Renderer/Public/Camera/RenderCamera.h"

PerViewConstantBufferData PerViewDataBuilder::BuildPerViewData(
	const PerViewCameraConstantBufferData& cameraData,
	const PerViewLightingConstantBufferData& lightingData) noexcept
{
	PerViewConstantBufferData perViewData{};
	perViewData.Camera = cameraData;
	perViewData.ViewLighting = lightingData;
	return perViewData;
}

RenderViewContext PerViewDataBuilder::BuildView(
	const PerViewCameraConstantBufferData& cameraData,
	const PerViewLightingConstantBufferData& lightingData,
	const D3D12_VIEWPORT& viewport,
	const D3D12_RECT& scissorRect) const noexcept
{
	RenderViewContext viewContext{};
	viewContext.perViewData = BuildPerViewData(cameraData, lightingData);
	viewContext.viewport = viewport;
	viewContext.scissorRect = scissorRect;
	return viewContext;
}

RenderViewContext PerViewDataBuilder::BuildMainView(
    const RenderCamera& renderCamera,
	    const PerViewLightingConstantBufferData& lightingData,
	const D3D12SwapChain& swapChain) const noexcept
{
	return BuildView(
	    renderCamera.GetCameraConstantBufferData(),
	    lightingData,
	    swapChain.GetDefaultViewport(),
	    swapChain.GetDefaultScissorRect());
}
