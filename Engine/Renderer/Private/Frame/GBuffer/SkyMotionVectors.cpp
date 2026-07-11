#include "PCH.h"
#include "Frame/GBuffer/SkyMotionVectors.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Deferred/SkyMotionVectorPass.h"

void AddSkyMotionVectorPass(FrameGraphBuilder& builder, const GBufferRenderTargets& targets)
{
	auto& parameters = builder.AllocPassParameters<SkyMotionVectorPass>();
	SkyMotionVectorPass::DeclareResources(builder, targets.DeviceZ, targets.MotionVector, parameters);
	builder.AddComputeShaderPass<SkyMotionVectorPass>(parameters);
}
