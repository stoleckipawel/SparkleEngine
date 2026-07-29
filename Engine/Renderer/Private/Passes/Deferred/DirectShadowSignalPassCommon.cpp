#include "../../PCH.h"
#include "Passes/Deferred/DirectShadowSignalPassCommon.h"

#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "Frame/Lighting/ShadowVisibility.h"
#include "FrameGraph/PassRuntimeContext.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"

void DirectShadowSignalPassCommon::SetParameters(
    DirectShadowSignalCommonPassParameters& parameters,
    const FrameContext& frame,
    const RenderViewData& viewData,
    const PassRuntimeContext& passRuntimeContext)
{
	parameters.PerFrame = passRuntimeContext.PerFrame;
	parameters.PerView = viewData.perViewData;
	parameters.PerTemporal = viewData.perTemporalData;
	parameters.ViewLighting = frame.sceneGpuData->Lighting.Constants;
}

void DirectShadowSignalPassCommon::SetRayQueryParameters(
    DirectShadowSignalRayQueryPassParameters& parameters,
    const FrameContext& frame,
    const RenderViewData& viewData,
    const PassRuntimeContext& passRuntimeContext,
    bool hasTraceableInstances)
{
	SetParameters(parameters, frame, viewData, passRuntimeContext);
	parameters.MaterialTextureSampler = RhiSamplerDesc{
	    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	    .MipFilter = RhiSamplerMipFilter::Linear,
	    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Wrap),
	    .MaxAnisotropy = RhiSamplerAnisotropy::X1};

	parameters.MaterialTextureTable = frame.sceneData.materialTextureTable.Binding;
	parameters.RayTracedShadows = RayTracedShadowPassData::Build(
	    passRuntimeContext.RayTracing,
	    hasTraceableInstances,
	    frame.sceneGpuData->RayTracing.InstanceCount,
	    frame.sceneGpuData->RayTracing.MaterialCount);
}
