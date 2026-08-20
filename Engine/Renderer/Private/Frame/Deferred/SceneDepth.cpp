#include "PCH.h"
#include "Frame/Deferred/SceneDepth.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Deferred/SceneDepthPass.h"
#include "View/RenderView.h"

void AddLinearizeDeviceZPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameGraphTextureHandle deviceZ,
    FrameGraphTextureHandle sceneDepth)
{
	auto& parameters = builder.AllocParameters<SceneDepthPass::Parameters>();
	auto* parameterFields = parameters.operator->();
	parameters->GBufferDeviceZ = builder.CreateSRV(deviceZ);
	parameters->SceneDepth = builder.CreateUAV(sceneDepth);
	builder.AddRenderViewSetup([parameterFields](const RenderView& view) { parameterFields->ViewCamera = view.cameraUniform; });
	builder.DispatchAsync<SceneDepthPass>(parameters, sceneExtent.Width, sceneExtent.Height);
}
