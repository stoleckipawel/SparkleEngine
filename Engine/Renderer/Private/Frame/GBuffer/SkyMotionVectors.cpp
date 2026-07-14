#include "PCH.h"
#include "Frame/GBuffer/SkyMotionVectors.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Deferred/SkyMotionVectorPass.h"

void AddSkyMotionVectorPass(FrameGraphBuilder& builder, const GBufferRenderTargets& targets)
{
	auto& parameters = builder.AllocParameters<SkyMotionVectorPass::Parameters>();
	parameters->GBufferDeviceZ = builder.CreateSRV(targets.DeviceZ);
	parameters->GBufferMotionVector = builder.CreateUAV(targets.MotionVector);
	builder.Dispatch<SkyMotionVectorPass>(parameters);
}
