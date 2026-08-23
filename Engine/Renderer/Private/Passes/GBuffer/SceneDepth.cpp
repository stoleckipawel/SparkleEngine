#include "PCH.h"
#include "Passes/GBuffer/SceneDepth.h"

#include "Core/Public/Math/MathUtils.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/GBuffer/SceneDepthShader.h"
#include "View/RenderView.h"

void AddLinearizeDeviceZPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameGraphTextureHandle deviceZ,
    FrameGraphTextureHandle sceneDepth)
{
	auto& parameters = builder.AllocParameters<SceneDepthCS>();
	parameters->GBufferDeviceZ = builder.CreateSRV(deviceZ);
	parameters->SceneDepth = builder.CreateUAV(sceneDepth);
	builder.AddParameterSetup<RenderView>(parameters, [](auto& fields, const RenderView& view) { fields.ViewCamera = view.cameraUniform; });
	builder.DispatchAsync<SceneDepthCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}
