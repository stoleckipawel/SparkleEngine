#pragma once

#include "Frame/Core/FrameContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/Bindings/MaterialTextureTablePassBinding.h"
#include "Passes/Bindings/RayTracingHitDataPassBinding.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"
#include "RayTracing/RayTracingPassCapabilityQuery.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"

namespace RayTracedShadowPassBinding
{
	template <typename TParameterInstance>
	void SetRayQueryParameters(
	    TParameterInstance& parameters,
	    const FrameContext& frame,
	    const PassRuntimeServices& passRuntimeServices,
	    bool hasSceneTlas)
	{
		RayTracingHitDataPassBinding::SetTriangleMaterialParameters(parameters, frame);
		parameters->MaterialTextureSampler =
		    RhiSamplerDesc{
		        .MinMagFilter = RhiSamplerMinMagFilter::Linear,
		        .MipFilter = RhiSamplerMipFilter::Linear,
		        .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Wrap),
		        .MaxAnisotropy = RhiSamplerAnisotropy::X1};

		const RayTracingPassCapabilities rayTracingCapabilities =
		    RayTracingPassCapabilityQuery::Build(frame, passRuntimeServices.RayTracing);
		const bool materialTextureTableAvailable = MaterialTextureTablePassBinding::Bind(parameters, frame);
		parameters->RayTracedShadows = RayTracedShadowPassData::Build(
		    passRuntimeServices.RayTracing,
		    hasSceneTlas,
		    rayTracingCapabilities.TriangleMaterialDataAvailable && materialTextureTableAvailable,
		    frame.rayTracingHitData.GetInstanceCount(),
		    frame.rayTracingHitData.GetMaterialCount());
	}
}
