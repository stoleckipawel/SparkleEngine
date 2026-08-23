#include "PCH.h"
#include "Passes/GBuffer/SkyMotionVectors.h"

#include "Core/Public/Math/MathUtils.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/GBuffer/SkyMotionVectorShader.h"
#include "View/RenderView.h"

void AddSkyMotionVectorPass(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, const GBufferRenderTargets& targets)
{
	auto& parameters = builder.AllocParameters<SkyMotionVectorCS>();
	parameters->GBufferDeviceZ = builder.CreateSRV(targets.DeviceZ);
	parameters->GBufferMotionVector = builder.CreateUAV(targets.MotionVector);
	builder.AddParameterSetup<RenderView>(
	    parameters,
	    [](auto& fields, const RenderView& view)
	    {
		    fields.View = view.uniform;
		    fields.ViewCamera = view.cameraUniform;
		    fields.ViewTemporal = view.temporalUniform;
	    });
	builder.DispatchAsync<SkyMotionVectorCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}
