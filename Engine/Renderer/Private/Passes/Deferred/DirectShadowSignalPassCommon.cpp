#include "../../PCH.h"
#include "Passes/Deferred/DirectShadowSignalPassCommon.h"

#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "Frame/Lighting/ShadowVisibility.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"
#include "RayTracing/RayTracingPassCapabilityQuery.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"
#include "SceneData/MaterialTextureTableCapability.h"

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
