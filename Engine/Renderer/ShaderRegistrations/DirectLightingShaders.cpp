#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"

#include "Resources/RenderConstantBufferData.h"
#include "Renderer/Private/RayTracing/RayTracedShadowUniformData.h"

class DirectLightingCS final : public TGlobalShader<DirectLightingCS>
{
  public:
	static constexpr CookedShaderPackageFeatureFlags kPackageFeatures =
	    CookedShaderPackageFeatureFlags::UsesAccelerationStructure | CookedShaderPackageFeatureFlags::UsesInlineRayQuery;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, DirectDiffuse, DirectDiffuseTexture)
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, DirectSpecular, DirectSpecularTexture)
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, DirectSubsurface, DirectSubsurfaceTexture)
	SHADER_PARAMETER_ACCELERATION_STRUCTURE(SceneTlas)
	SHADER_PARAMETER_CBUFFER_NAMED(PerFrame, PerFrameConstantBufferData, PerFrameConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(PerView, PerViewConstantBufferData, PerViewConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(RayTracedShadows, RayTracedShadowUniformData, RayTracedShadowUniformData)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferBaseColor)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferNormal)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferMaterial)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferSubsurface)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferDeviceZ)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    DirectLightingCS,
    RendererShaderPackages::DirectLighting,
    "Passes/Deferred/DirectLighting.hlsl",
    "main",
    Compute);
