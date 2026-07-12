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
    DirectShadowSignalCommonPassParameters& parameters)
{
	parameters.ShadowVisibilitySignal = builder.CreateUAV(shadowSignals.Visibility);
	parameters.CurrentReservoirSample = builder.CreateSRV(shadowSignals.CurrentReservoirSample);
	parameters.CurrentReservoirWeight = builder.CreateSRV(shadowSignals.CurrentReservoirWeight);
	parameters.SceneDepth = builder.CreateSRV(sceneDepth);
}

void DirectShadowSignalPassCommon::DeclareRayQueryResources(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle sceneDepth,
    const GBufferRenderTargets& gbuffer,
    const DirectShadowSignalResources& shadowSignals,
    DirectShadowSignalRayQueryPassParameters& parameters)
{
	DeclareResources(builder, sceneDepth, shadowSignals, parameters);
	parameters.GBufferNormal = builder.CreateSRV(gbuffer.Normal);
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
	parameters.DirectionalLights = frame.lighting.GetDirectionalLightsShaderResourceView();
	parameters.PointLights = frame.lighting.GetPointLightsShaderResourceView();
	parameters.SpotLights = frame.lighting.GetSpotLightsShaderResourceView();
	parameters.RectLights = frame.lighting.GetRectLightsShaderResourceView();
}

void DirectShadowSignalPassCommon::SetRayQueryParameters(
    DirectShadowSignalRayQueryPassParameters& parameters,
    const FrameContext& frame,
    const RenderViewData& viewData,
    const PassRuntimeServices& passRuntimeServices,
    bool hasSceneTlas)
{
	SetParameters(parameters, frame, viewData, passRuntimeServices);
	parameters.RayTracingHitVertices = frame.rayTracingHitData.GetVertexShaderResourceView();
	parameters.RayTracingHitIndices = frame.rayTracingHitData.GetIndexShaderResourceView();
	parameters.RayTracingHitInstances = frame.rayTracingHitData.GetInstanceShaderResourceView();
	parameters.RayTracingHitMaterials = frame.rayTracingHitData.GetMaterialShaderResourceView();
	parameters.MaterialTextureSampler = RhiSamplerDesc{
	    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	    .MipFilter = RhiSamplerMipFilter::Linear,
	    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Wrap),
	    .MaxAnisotropy = RhiSamplerAnisotropy::X1};

	const RayTracingPassCapabilities capabilities = RayTracingPassCapabilityQuery::Build(frame, passRuntimeServices.RayTracing);
	const RenderBindingSet* materialTextureTable = frame.sceneData.materialTextureTable;
	const std::uint32_t descriptorCount = frame.sceneData.materialTextureTableDescriptorCount;
	const bool materialTextureTableAvailable =
	    frame.sceneData.materialTextureTableValid && materialTextureTable != nullptr && *materialTextureTable &&
	    descriptorCount > 0u && descriptorCount <= MaterialTextureTableFixedCapacity &&
	    materialTextureTable->GetDescriptorCount() >= descriptorCount;
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
