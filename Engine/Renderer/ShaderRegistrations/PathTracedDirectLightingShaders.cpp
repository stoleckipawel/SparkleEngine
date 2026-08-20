#include "PCH.h"
#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"
#include "Renderer/Private/RayTracing/Effects/PathTracedLighting/PathTracedLightingUniformData.h"
#include "Renderer/Private/RayTracing/Effects/Shadows/RayTracedShadowUniformData.h"
#include "Renderer/Private/RayTracing/RayTracingHitData.h"
#include "Renderer/Private/RayTracing/RayTracingShaderFeatureFlags.h"
#include "Renderer/Private/Scene/Materials/MaterialTextureTableCapability.h"
#include "ShaderData/RenderConstantBufferData.h"
#include "ShaderData/RenderViewLightingData.h"

class PathTracedDirectLightingCS final : public TGlobalShader<PathTracedDirectLightingCS>
{
public:
	static constexpr CookedShaderPackageFeatureFlags kPackageFeatures = RayTracingShaderFeatureFlags::DescriptorRayQuery;
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV(RWTexture2D, DirectDiffuse)
	SHADER_PARAMETER_UAV(RWTexture2D, DirectSpecular)
	SHADER_PARAMETER_UAV(RWTexture2D, DirectSubsurface)
	SHADER_PARAMETER_ACCELERATION_STRUCTURE(SceneTlas)
	SHADER_PARAMETER_CBUFFER_NAMED(PerFrame, PerFrameConstantBufferData, PerFrameConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(PerView, PerViewConstantBufferData, PerViewConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(PerTemporal, PerTemporalConstantBufferData, PerTemporalConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(ViewLighting, ViewLighting, ViewLightingData)
	SHADER_PARAMETER_CBUFFER_NAMED(RayTracedShadows, RayTracedShadowUniformData, RayTracedShadowUniformData)
	SHADER_PARAMETER_CBUFFER_NAMED(PathTracedLightingConstants, PathTracedLightingUniformData, PathTracedLightingUniformData)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferBaseColor)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferNormal)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferMaterial)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferSubsurface)
	SHADER_PARAMETER_TEXTURE(Texture2D, SceneDepth)
	SHADER_PARAMETER_RDG_BUFFER_SRV(DirectionalLightConstantBufferData, DirectionalLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(PointLightConstantBufferData, PointLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(SpotLightConstantBufferData, SpotLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RectLightConstantBufferData, RectLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RayTracingHitVertex, RayTracingHitVertices)
	SHADER_PARAMETER_RDG_BUFFER_SRV(uint32_t, RayTracingHitIndices)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RayTracingHitInstance, RayTracingHitInstances)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RayTracingHitMaterial, RayTracingHitMaterials)
	SHADER_PARAMETER_TEXTURE_ARRAY(Texture2D, MaterialTextureTable, MaterialTextureTableFixedCapacity)
	SHADER_PARAMETER_SAMPLER(SamplerState, MaterialTextureSampler)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    PathTracedDirectLightingCS,
    RendererShaderPackages::PathTracedDirectLighting,
    "Passes/RayTracing/PathTracedDirectLighting.hlsl",
    "main",
    Compute);
