#include "../../PCH.h"
#include "Frame/Lighting/DirectLightReservoir.h"

#include "Frame/Lighting/ShadowVisibility.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Deferred/DirectLightReservoirPass.h"

void AddDirectLightReservoirPasses(
    FrameGraphBuilder& builder,
    const GBufferRenderTargets& gbuffer,
    const DirectShadowSignalResources& shadowSignals)
{
	auto& temporalParameters = builder.AllocPassParameters<DirectLightReservoirTemporalPass>();
	DirectLightReservoirTemporalPass::DeclareResources(builder, gbuffer, shadowSignals, temporalParameters);
	builder.AddComputeShaderPass<DirectLightReservoirTemporalPass>(temporalParameters);

	auto& spatialParameters = builder.AllocPassParameters<DirectLightReservoirSpatialPass>();
	DirectLightReservoirSpatialPass::DeclareResources(builder, gbuffer, shadowSignals, spatialParameters);
	builder.AddComputeShaderPass<DirectLightReservoirSpatialPass>(spatialParameters);
}
