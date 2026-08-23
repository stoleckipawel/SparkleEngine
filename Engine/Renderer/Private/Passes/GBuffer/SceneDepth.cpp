#include "PCH.h"
#include "Passes/GBuffer/SceneDepth.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/GBuffer/SceneDepthPass.h"
#include "View/RenderView.h"

void AddLinearizeDeviceZPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameGraphTextureHandle deviceZ,
    FrameGraphTextureHandle sceneDepth)
{
	auto& parameters = builder.AllocParameters<SceneDepthPass::Parameters>();
	parameters->GBufferDeviceZ = builder.CreateSRV(deviceZ);
	parameters->SceneDepth = builder.CreateUAV(sceneDepth);
	builder.AddParameterSetup<RenderView>(
	    parameters,
	    [](auto& fields, const RenderView& view) { fields.ViewCamera = view.cameraUniform; });
	builder.DispatchAsync<SceneDepthPass>(parameters, sceneExtent.Width, sceneExtent.Height);
}
