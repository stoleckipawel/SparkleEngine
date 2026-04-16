#include "PCH.h"

#include "PerViewDataBuilder.h"

#include "Camera/RenderCamera.h"
#include "RHI/Public/Interop/RenderHardwareInterface.h"

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
	const RhiViewport& viewport,
	const RhiRect& scissorRect) const noexcept
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
    const RenderHardwareInterface& renderHardwareInterface) const noexcept
{
	return BuildView(
	    renderCamera.GetCameraConstantBufferData(),
	    lightingData,
	    renderHardwareInterface.GetBackBufferViewport(),
	    renderHardwareInterface.GetBackBufferScissorRect());
}
