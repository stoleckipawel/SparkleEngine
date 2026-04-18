#include "PCH.h"
#include "Frame/Builders/BuildFrameContext.h"
#include "Frame/FrameContext.h"

#include "RHI/Public/Interop/RenderHardwareInterface.h"
#include "RHI/Public/Resources/RenderViewLightingData.h"
#include "Frame/Builders/PerViewDataBuilder.h"
#include "Frame/Shadow/ShadowBuilder.h"
#include "Frame/Shadow/ShadowFrameBuilder.h"
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
    ViewLightingBuilder& viewLightingBuilder,
    ShadowFrameBuilder& shadowFrameBuilder,
    ShadowBuilder& shadowBuilder)
{
	FrameContext frame{};
	frame.sceneData = renderSceneDataBuilder.Build(sceneSnapshot);
	const PerViewLightingConstantBufferData baseLighting = viewLightingBuilder.Build(frame.sceneData);
	ShadowFrameBuildResult shadowFrame =
	    shadowFrameBuilder
	        .Build(sceneSnapshot.camera, frame.sceneData, baseLighting, renderHardwareInterface, perViewDataBuilder, shadowBuilder);
	frame.shadowViews = shadowFrame.shadowViews;
	frame.shadowViewCount = shadowFrame.shadowViewCount;
	frame.mainView = perViewDataBuilder.BuildMainView(renderCamera, shadowFrame.mainViewLighting, renderHardwareInterface);

	frame.mainView.perViewGpuAddress = renderHardwareInterface.AllocatePerViewConstantBuffer(frame.mainView.perViewData);

	return frame;
}
