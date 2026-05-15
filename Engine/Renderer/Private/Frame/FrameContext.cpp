#include "PCH.h"
#include "Frame/Builders/BuildFrameContext.h"
#include "Frame/FrameContext.h"

#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Resources/RenderViewLightingData.h"
#include "Frame/Builders/PerViewDataBuilder.h"
#include "Frame/Builders/ViewLightingBuilder.h"
#include "Camera/RenderCamera.h"
#include "SceneData/Builders/RenderSceneDataBuilder.h"
#include "SceneData/Lifecycle/RenderSceneSnapshot.h"

#include <cstdio>
#include <utility>

FrameContext BuildFrameContext(
    const RenderSceneSnapshot& sceneSnapshot,
    RenderHardwareInterface& renderHardwareInterface,
    const RenderCamera& renderCamera,
    RenderSceneDataBuilder& renderSceneDataBuilder,
    PerViewDataBuilder& perViewDataBuilder,
    ViewLightingBuilder& viewLightingBuilder)
{
	FrameContext frame{};
	frame.sceneData = renderSceneDataBuilder.Build(sceneSnapshot);
	const PerViewLightingConstantBufferData lighting = viewLightingBuilder.Build(frame.sceneData);
	frame.mainView = perViewDataBuilder.BuildMainView(renderCamera, lighting, renderHardwareInterface);

	frame.mainView.perViewGpuAddress = renderHardwareInterface.AllocatePerViewConstantBuffer(frame.mainView.perViewData);

	return frame;
}
