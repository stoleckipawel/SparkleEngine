#include "PCH.h"
#include "Frame/Deferred/SceneDepth.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Deferred/SceneDepthPass.h"

void AddLinearizeDeviceZPass(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle deviceZ,
    FrameGraphTextureHandle sceneDepth)
{
	auto& parameters = builder.AllocPassParameters<SceneDepthPass>();
	SceneDepthPass::DeclareResources(builder, deviceZ, sceneDepth, parameters);
	builder.AddComputeShaderPass<SceneDepthPass>(parameters);
}
