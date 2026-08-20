#include "PCH.h"
#include "Frame/GBuffer/SkyMotionVectors.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Deferred/SkyMotionVectorPass.h"
#include "View/RenderView.h"

void AddSkyMotionVectorPass(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, const GBufferRenderTargets& targets)
{
	auto& parameters = builder.AllocParameters<SkyMotionVectorPass::Parameters>();
	auto* parameterFields = parameters.operator->();
	parameters->GBufferDeviceZ = builder.CreateSRV(targets.DeviceZ);
	parameters->GBufferMotionVector = builder.CreateUAV(targets.MotionVector);
	builder.AddRenderViewSetup(
	    [parameterFields](const RenderView& view)
	    {
		    parameterFields->View = view.uniform;
		    parameterFields->ViewCamera = view.cameraUniform;
		    parameterFields->ViewTemporal = view.temporalUniform;
	    });
	builder.DispatchAsync<SkyMotionVectorPass>(parameters, sceneExtent.Width, sceneExtent.Height);
}
