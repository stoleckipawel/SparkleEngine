#pragma once

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassInput.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowUniformData.h"

namespace RayTracedShadowPassData
{
	RayTracedShadowUniformData Build(const RayTracedShadowPassInput& input) noexcept;
}

template <typename TParameterInstance> void BindRayTracedShadowParameters(FrameGraphBuilder& builder, TParameterInstance& parameters)
{
	builder.AddParameterSetup<RayTracedShadowPassInput>(
	    parameters,
	    [](auto& fields, const RayTracedShadowPassInput& input)
	    { fields.RayTracedShadowConstants = RayTracedShadowPassData::Build(input); });
}
