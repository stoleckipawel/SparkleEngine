#include "../../PCH.h"
#include "Frame/Lighting/DirectLightReservoir.h"

#include "Frame/Lighting/ShadowVisibility.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Deferred/DirectLightReservoirPass.h"

void AddDirectLightReservoirPasses(
    FrameGraphBuilder& builder,
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
    const DirectShadowSignalResources& shadowSignals)
{
	auto& temporalParameters = builder.AllocPassParameters<DirectLightReservoirTemporalPass>();
	DirectLightReservoirTemporalPass::DeclareResources(builder, sceneTargets.SceneDepth, gbuffer, shadowSignals, temporalParameters);
	builder.AddComputeShaderPass<DirectLightReservoirTemporalPass>(temporalParameters);

	auto& spatialParameters = builder.AllocPassParameters<DirectLightReservoirSpatialPass>();
	DirectLightReservoirSpatialPass::DeclareResources(builder, sceneTargets.SceneDepth, gbuffer, shadowSignals, spatialParameters);
	builder.AddComputeShaderPass<DirectLightReservoirSpatialPass>(spatialParameters);
}
