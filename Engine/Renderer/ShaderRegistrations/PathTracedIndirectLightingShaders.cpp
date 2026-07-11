#include "PCH.h"

#include "RendererShaderPackages.h"
#include "RayTracedSurfaceLightingShaderParameters.h"
#include "Shaders/Authoring/GlobalShader.h"

#include "Renderer/Private/RayTracing/Effects/PathTracedLighting/PathTracedLightingUniformData.h"

class PathTracedIndirectLightingCS final : public TGlobalShader<PathTracedIndirectLightingCS>
{
  public:
	static constexpr CookedShaderPackageFeatureFlags kPackageFeatures = RayTracedSurfaceLightingShaderParameters::PackageFeatures;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV(RWTexture2D, IndirectDiffuse)
	SHADER_PARAMETER_UAV(RWTexture2D, IndirectSpecular)
	RAY_TRACED_SURFACE_LIGHTING_SHADER_PARAMETERS()
	SHADER_PARAMETER_CBUFFER_NAMED(PathTracedLightingConstants, PathTracedLightingUniformData, PathTracedLightingUniformData)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    PathTracedIndirectLightingCS,
    RendererShaderPackages::PathTracedIndirectLighting,
    "Passes/RayTracing/PathTracedIndirectLighting.hlsl",
    "main",
    Compute);
