#include "../../PCH.h"
#include "Frame/Lighting/DirectLightReservoir.h"

#include "Frame/Lighting/ShadowVisibility.h"
#include "Frame/Core/FrameAssembly.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Deferred/DirectLightReservoirSpatialPass.h"
#include "Passes/Deferred/DirectLightReservoirTemporalPass.h"

void AddDirectLightReservoirPasses(
    FrameGraphBuilder& builder,
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
    const DirectShadowSignalResources& shadowSignals,
    const FrameAssemblyExternalResources& externalResources)
{
	auto& temporalParameters = builder.AllocPassParameters<DirectLightReservoirTemporalPass>();
	DirectLightReservoirTemporalPass::DeclareResources(
	    builder,
	    sceneTargets.SceneDepth,
	    gbuffer,
	    shadowSignals,
	    externalResources.DirectionalLights,
	    externalResources.PointLights,
	    externalResources.SpotLights,
	    externalResources.RectLights,
	    temporalParameters);
	builder.AddComputeShaderPass<DirectLightReservoirTemporalPass>(temporalParameters);

	auto& spatialParameters = builder.AllocPassParameters<DirectLightReservoirSpatialPass>();
	DirectLightReservoirSpatialPass::DeclareResources(
	    builder,
	    sceneTargets.SceneDepth,
	    gbuffer,
	    shadowSignals,
	    externalResources.DirectionalLights,
	    externalResources.PointLights,
	    externalResources.SpotLights,
	    externalResources.RectLights,
	    spatialParameters);
	builder.AddComputeShaderPass<DirectLightReservoirSpatialPass>(spatialParameters);
}
