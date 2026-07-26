#include "../../PCH.h"
#include "Passes/Deferred/DirectShadowSignalPassCommon.h"

#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "Frame/Lighting/ShadowVisibility.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"
#include "RayTracing/RayTracingPassCapabilityQuery.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"

void DirectShadowSignalPassCommon::SetParameters(
    DirectShadowSignalCommonPassParameters& parameters,
    const FrameContext& frame,
    const RenderViewData& viewData,
    const PassRuntimeServices& passRuntimeServices)
{
	parameters.PerFrame = passRuntimeServices.PerFrame;
	parameters.PerView = viewData.perViewData;
	parameters.PerTemporal = viewData.perTemporalData;
	parameters.ViewLighting = frame.sceneGpuData->Lighting.Constants;
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
	const bool materialTextureTableAvailable = static_cast<bool>(frame.sceneData.materialTextureTable);
	if (materialTextureTableAvailable)
	{
		parameters.MaterialTextureTable = frame.sceneData.materialTextureTable.Binding;
	}
	parameters.RayTracedShadows = RayTracedShadowPassData::Build(
	    passRuntimeServices.RayTracing,
	    hasSceneTlas,
	    capabilities.TriangleMaterialDataAvailable && materialTextureTableAvailable,
	    frame.sceneGpuData->RayTracing.InstanceCount,
	    frame.sceneGpuData->RayTracing.MaterialCount);
}
