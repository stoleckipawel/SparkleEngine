#include "PCH.h"
#include "BuildFrameContext.h"
#include "Renderer/Public/FrameContext.h"

#include "D3D12ConstantBufferManager.h"
#include "D3D12SwapChain.h"
#include "D3D12ViewLightingConstantBufferData.h"
#include "Frame/PerViewDataBuilder.h"
#include "Frame/Shadow/ShadowBuilder.h"
#include "Frame/Shadow/ShadowFrameBuilder.h"
#include "Frame/ViewLightingBuilder.h"
#include "Renderer/Public/Camera/RenderCamera.h"
#include "Core/Public/Diagnostics/Log.h"
#include "SceneData/RenderSceneDataBuilder.h"
#include "SceneData/RenderSceneSnapshot.h"

#include <cstdio>
#include <utility>

FrameContext BuildFrameContext(
	const RenderSceneSnapshot& sceneSnapshot,
    const D3D12SwapChain& swapChain,
    D3D12ConstantBufferManager& constantBufferManager,
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
	ShadowFrameBuildResult shadowFrame = shadowFrameBuilder.Build(
	    sceneSnapshot.camera,
	    frame.sceneData,
	    baseLighting,
	    constantBufferManager,
	    perViewDataBuilder,
	    shadowBuilder);
	frame.shadowViews = shadowFrame.shadowViews;
	frame.shadowViewCount = shadowFrame.shadowViewCount;
	frame.mainView = perViewDataBuilder.BuildMainView(renderCamera, shadowFrame.mainViewLighting, swapChain);

	frame.mainView.perViewGpuAddress = constantBufferManager.AllocatePerView(frame.mainView.perViewData);

	return frame;
}
