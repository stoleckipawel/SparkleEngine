#include "PCH.h"

#include "PerViewDataBuilder.h"

#include "Camera/RenderCamera.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"

PerViewConstantBufferData PerViewDataBuilder::BuildPerViewData(
    const PerViewCameraConstantBufferData& cameraData,
    const PerViewLightingConstantBufferData& lightingData) noexcept
{
	PerViewConstantBufferData perViewData{};
	perViewData.Camera = cameraData;
	perViewData.ViewLighting = lightingData;
	return perViewData;
}

RenderViewData PerViewDataBuilder::BuildView(
    const PerViewCameraConstantBufferData& cameraData,
    const PerViewLightingConstantBufferData& lightingData,
    const RhiViewport& viewport,
    const RhiRect& scissorRect) const noexcept
{
	RenderViewData viewData{};
	viewData.perViewData = BuildPerViewData(cameraData, lightingData);
	viewData.viewport = viewport;
	viewData.scissorRect = scissorRect;
	return viewData;
}

RenderViewData PerViewDataBuilder::BuildMainView(
    const RenderCamera& renderCamera,
    const PerViewLightingConstantBufferData& lightingData,
    const RenderHardwareInterface& renderHardwareInterface) const noexcept
{
	return BuildView(
	    renderCamera.GetCameraConstantBufferData(),
	    lightingData,
	    renderHardwareInterface.GetPresentationService().GetBackBufferViewport(),
	    renderHardwareInterface.GetPresentationService().GetBackBufferScissorRect());
}
