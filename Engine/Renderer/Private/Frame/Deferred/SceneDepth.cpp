#include "PCH.h"
#include "Frame/Deferred/SceneDepth.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Deferred/SceneDepthPass.h"

void AddLinearizeDeviceZPass(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle deviceZ,
    FrameGraphTextureHandle sceneDepth)
{
	auto& parameters = builder.AllocParameters<SceneDepthPass::Parameters>();
	parameters->GBufferDeviceZ = builder.CreateSRV(deviceZ);
	parameters->SceneDepth = builder.CreateUAV(sceneDepth);
	builder.DispatchAsync<SceneDepthPass>(parameters);
}
