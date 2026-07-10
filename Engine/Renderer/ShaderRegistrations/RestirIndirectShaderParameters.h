#pragma once

#include "RayTracedSurfaceLightingShaderParameters.h"
#include "Renderer/Private/RayTracing/Effects/RestirLighting/RestirIndirectLightingUniformData.h"

namespace RestirIndirectShaderParameters
{
	inline constexpr CookedShaderPackageFeatureFlags PackageFeatures = RayTracedSurfaceLightingShaderParameters::PackageFeatures;
}

#define RESTIR_INDIRECT_SHADER_PARAMETERS()         \
	RAY_TRACED_SURFACE_LIGHTING_SHADER_PARAMETERS() \
	SHADER_PARAMETER_CBUFFER_NAMED(RestirIndirectConstants, RestirIndirectLightingUniformData, RestirIndirectLightingUniformData)
