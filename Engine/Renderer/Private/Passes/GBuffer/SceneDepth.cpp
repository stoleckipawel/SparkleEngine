#include "PCH.h"
#include "Passes/GBuffer/SceneDepth.h"

#include "Core/Public/Math/MathUtils.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/GBuffer/SceneDepthShader.h"
#include "View/RenderView.h"

void AddLinearizeDeviceZPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RenderFrameGraphResources& resources)
{
	auto& parameters = builder.AllocParameters<SceneDepthCS>();
	parameters->GBufferDeviceZ = builder.CreateSRV(resources.Transient.GBuffer.DeviceZ);
	parameters->SceneDepth = builder.CreateUAV(resources.Transient.Scene.SceneDepth);
	builder.AddParameterSetup<RenderView>(parameters, [](auto& fields, const RenderView& view) { fields.ViewCamera = view.cameraUniform; });
	builder.DispatchAsync<SceneDepthCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}
