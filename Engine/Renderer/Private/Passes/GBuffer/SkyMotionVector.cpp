#include "PCH.h"
#include "Passes/GBuffer/SkyMotionVector.h"

#include "Frame/RenderFrame.h"
#include "Core/Public/Math/MathUtils.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/GBuffer/SkyMotionVectorShader.h"
#include "Passes/Lighting/Sky/Sky.h"
#include "View/RenderView.h"

void AddSkyMotionVectorPass(
    FrameGraphBuilder& builder,
    const RenderFrame& frame,
    RenderViewportExtent sceneExtent,
    const RenderFrameGraphResources& resources)
{
	if (!CVarSkyEnabled.Get())
	{
		return;
	}
	const GBufferRenderTargets& targets = resources.Transient.GBuffer;

	auto& parameters = builder.AllocParameters<SkyMotionVectorCS>();
	parameters->GBufferDeviceZ = builder.CreateSRV(targets.DeviceZ);
	parameters->GBufferMotionVector = builder.CreateUAV(targets.MotionVector);

	parameters->View = frame.View.uniform;
	parameters->ViewCamera = frame.View.cameraUniform;
	parameters->ViewTemporal = frame.View.temporalUniform;

	builder.Dispatch<SkyMotionVectorCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u},
	    EFrameGraphQueuePreference::AsyncCompute);
}
