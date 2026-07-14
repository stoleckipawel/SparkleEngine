#include "../../PCH.h"
#include "Passes/Deferred/DirectShadowSignalPassCommon.h"

#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "Frame/Lighting/ShadowVisibility.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"
#include "RayTracing/RayTracingPassCapabilityQuery.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"
#include "SceneData/MaterialTextureTableCapability.h"

void DirectShadowSignalPassCommon::DeclareResources(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle sceneDepth,
    const DirectShadowSignalResources& shadowSignals,
    FrameGraphBufferHandle directionalLights,
    FrameGraphBufferHandle pointLights,
    FrameGraphBufferHandle spotLights,
    FrameGraphBufferHandle rectLights,
    DirectShadowSignalCommonPassParameters& parameters)
{
	parameters.ShadowVisibilitySignal = builder.CreateUAV(shadowSignals.Visibility);
	parameters.CurrentReservoirSample = builder.CreateSRV(shadowSignals.ReservoirHistory.Sample.Current);
	parameters.CurrentReservoirWeight = builder.CreateSRV(shadowSignals.ReservoirHistory.Weight.Current);
	parameters.SceneDepth = builder.CreateSRV(sceneDepth);
	parameters.DirectionalLights = builder.CreateSRV(directionalLights);
	parameters.PointLights = builder.CreateSRV(pointLights);
	parameters.SpotLights = builder.CreateSRV(spotLights);
	parameters.RectLights = builder.CreateSRV(rectLights);
}

void DirectShadowSignalPassCommon::DeclareRayQueryResources(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle sceneDepth,
    const GBufferRenderTargets& gbuffer,
    const DirectShadowSignalResources& shadowSignals,
    FrameGraphBufferHandle directionalLights,
    FrameGraphBufferHandle pointLights,
    FrameGraphBufferHandle spotLights,
    FrameGraphBufferHandle rectLights,
    FrameGraphBufferHandle hitVertices,
    FrameGraphBufferHandle hitIndices,
    FrameGraphBufferHandle hitInstances,
    FrameGraphBufferHandle hitMaterials,
    DirectShadowSignalRayQueryPassParameters& parameters)
{
	DeclareResources(builder, sceneDepth, shadowSignals, directionalLights, pointLights, spotLights, rectLights, parameters);
	parameters.GBufferNormal = builder.CreateSRV(gbuffer.Normal);
	parameters.RayTracingHitVertices = builder.CreateSRV(hitVertices);
	parameters.RayTracingHitIndices = builder.CreateSRV(hitIndices);
	parameters.RayTracingHitInstances = builder.CreateSRV(hitInstances);
	parameters.RayTracingHitMaterials = builder.CreateSRV(hitMaterials);
}

void DirectShadowSignalPassCommon::SetParameters(
    DirectShadowSignalCommonPassParameters& parameters,
    const FrameContext& frame,
    const RenderViewData& viewData,
    const PassRuntimeServices& passRuntimeServices)
{
	parameters.PerFrame = passRuntimeServices.PerFrame;
	parameters.PerView = viewData.perViewData;
	parameters.PerTemporal = viewData.perTemporalData;
	parameters.ViewLighting = frame.lighting.GetConstants();
}

void DirectShadowSignalPassCommon::SetRayQueryParameters(
    DirectShadowSignalRayQueryPassParameters& parameters,
    const FrameContext& frame,
    const RenderViewData& viewData,
    const PassRuntimeServices& passRuntimeServices,
    bool hasSceneTlas)
{
	SetParameters(parameters, frame, viewData, passRuntimeServices);
	parameters.MaterialTextureSampler = RhiSamplerDesc{
	    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	    .MipFilter = RhiSamplerMipFilter::Linear,
	    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Wrap),
	    .MaxAnisotropy = RhiSamplerAnisotropy::X1};

	const RayTracingPassCapabilities capabilities = RayTracingPassCapabilityQuery::Build(frame, passRuntimeServices.RayTracing);
	const RenderBindingSet* materialTextureTable = frame.sceneData.materialTextureTable;
	const std::uint32_t descriptorCount = frame.sceneData.materialTextureTableDescriptorCount;
	const bool materialTextureTableAvailable =
	    frame.sceneData.materialTextureTableValid && materialTextureTable != nullptr && *materialTextureTable && descriptorCount > 0u &&
	    descriptorCount <= MaterialTextureTableFixedCapacity && materialTextureTable->GetDescriptorCount() >= descriptorCount;
	if (materialTextureTableAvailable)
	{
		parameters.MaterialTextureTable = materialTextureTable->GetTableBinding(0);
	}
	parameters.RayTracedShadows = RayTracedShadowPassData::Build(
	    passRuntimeServices.RayTracing,
	    hasSceneTlas,
	    capabilities.TriangleMaterialDataAvailable && materialTextureTableAvailable,
	    frame.rayTracingHitData.GetInstanceCount(),
	    frame.rayTracingHitData.GetMaterialCount());
}
