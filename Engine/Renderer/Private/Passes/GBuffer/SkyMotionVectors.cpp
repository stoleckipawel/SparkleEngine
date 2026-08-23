#include "PCH.h"
#include "Passes/GBuffer/SkyMotionVectors.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/GBuffer/SkyMotionVectorPass.h"
#include "View/RenderView.h"

void AddSkyMotionVectorPass(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, const GBufferRenderTargets& targets)
{
	auto& parameters = builder.AllocParameters<SkyMotionVectorPass::Parameters>();
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
	builder.DispatchAsync<SkyMotionVectorPass>(parameters, sceneExtent.Width, sceneExtent.Height);
}
