#include "PCH.h"
#include "Passes/GBuffer/SkyMotionVector.h"

#include "Core/Public/Math/MathUtils.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/GBuffer/SkyMotionVectorShader.h"
#include "View/RenderView.h"

void AddSkyMotionVectorPass(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, const RenderFrameGraphResources& resources)
{
	const GBufferRenderTargets& targets = resources.Transient.GBuffer;

	auto& parameters = builder.AllocParameters<SkyMotionVectorCS>();
	parameters->GBufferDeviceZ = builder.CreateSRV(targets.DeviceZ);
	parameters->GBufferMotionVector = builder.CreateUAV(targets.MotionVector);

	builder.AddParameterSetup<RenderView>(
	    parameters,
	    [](auto& parameters, const RenderView& view)
	    {
		    parameters->View = view.uniform;
		    parameters->ViewCamera = view.cameraUniform;
		    parameters->ViewTemporal = view.temporalUniform;
	    });

	builder.DispatchAsync<SkyMotionVectorCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}
