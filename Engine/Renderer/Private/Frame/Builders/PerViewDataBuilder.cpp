#include "PCH.h"

#include "PerViewDataBuilder.h"

#include "Camera/RenderCamera.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"

PerViewConstantBufferData PerViewDataBuilder::BuildPerViewData(const PerViewCameraConstantBufferData& cameraData) noexcept
{
	PerViewConstantBufferData perViewData{};
	perViewData.Camera = cameraData;
	return perViewData;
}

RenderViewData PerViewDataBuilder::BuildView(
    const PerViewCameraConstantBufferData& cameraData,
    const RhiViewport& viewport,
    const RhiRect& scissorRect) const noexcept
{
	RenderViewData viewData{};
	viewData.perViewData = BuildPerViewData(cameraData);
	viewData.viewport = viewport;
	viewData.scissorRect = scissorRect;
	return viewData;
}

RenderViewData PerViewDataBuilder::BuildMainView(
    const RenderCamera& renderCamera,
    const RenderHardwareInterface& renderHardwareInterface) const noexcept
{
	return BuildView(
	    renderCamera.GetCameraConstantBufferData(),
	    renderHardwareInterface.GetPresentationService().GetBackBufferViewport(),
	    renderHardwareInterface.GetPresentationService().GetBackBufferScissorRect());
}
