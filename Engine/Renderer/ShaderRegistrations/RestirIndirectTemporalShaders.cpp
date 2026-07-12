#include "PCH.h"
#include "RendererShaderPackages.h"
#include "Renderer/Private/RayTracing/Effects/RestirLighting/RestirIndirectLightingUniformData.h"
#include "Renderer/Private/RayTracing/Effects/Shadows/RayTracedShadowUniformData.h"
#include "Renderer/Private/RayTracing/RayTracingHitData.h"
#include "Renderer/Private/RayTracing/RayTracingShaderFeatureFlags.h"
#include "Renderer/Private/SceneData/MaterialTextureTableCapability.h"
#include "Shaders/Authoring/GlobalShader.h"
#include "ShaderData/RenderConstantBufferData.h"
#include "ShaderData/RenderViewLightingData.h"

class RestirIndirectTemporalCS final : public TGlobalShader<RestirIndirectTemporalCS>
{
  public:
	static constexpr CookedShaderPackageFeatureFlags kPackageFeatures = RayTracingShaderFeatureFlags::DescriptorRayQuery;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV(RWTexture2D, TemporalReservoirSampleTexture)
	SHADER_PARAMETER_UAV(RWTexture2D, TemporalReservoirWeightTexture)
	SHADER_PARAMETER_TEXTURE(Texture2D, PreviousReservoirSampleTexture)
	SHADER_PARAMETER_TEXTURE(Texture2D, PreviousReservoirWeightTexture)
	SHADER_PARAMETER_TEXTURE(Texture2D, PreviousReservoirSurfaceTexture)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferMotionVector)
	SHADER_PARAMETER_ACCELERATION_STRUCTURE(SceneTlas)
	SHADER_PARAMETER_CBUFFER_NAMED(PerFrame, PerFrameConstantBufferData, PerFrameConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(PerView, PerViewConstantBufferData, PerViewConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(PerTemporal, PerTemporalConstantBufferData, PerTemporalConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(ViewLighting, ViewLighting, ViewLightingData)
	SHADER_PARAMETER_CBUFFER_NAMED(RayTracedShadows, RayTracedShadowUniformData, RayTracedShadowUniformData)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferBaseColor)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferNormal)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferMaterial)
	SHADER_PARAMETER_TEXTURE(Texture2D, SceneDepth)
	SHADER_PARAMETER_TEXTURE(Texture2D, SkyTexture)
	SHADER_PARAMETER_SAMPLER(SamplerState, SamplerLinearClamp)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RayTracingHitVertex, RayTracingHitVertices)
	SHADER_PARAMETER_RDG_BUFFER_SRV(uint32_t, RayTracingHitIndices)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RayTracingHitInstance, RayTracingHitInstances)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RayTracingHitMaterial, RayTracingHitMaterials)
	SHADER_PARAMETER_RDG_BUFFER_SRV(MeshInstanceData, MeshInstances)
	SHADER_PARAMETER_RDG_BUFFER_SRV(VertexSkinInfluenceData, SkinInfluences)
	SHADER_PARAMETER_RDG_BUFFER_SRV(JointMatrixData, JointMatrices)
	SHADER_PARAMETER_RDG_BUFFER_SRV(DirectionalLightConstantBufferData, DirectionalLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(PointLightConstantBufferData, PointLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(SpotLightConstantBufferData, SpotLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RectLightConstantBufferData, RectLights)
	SHADER_PARAMETER_TEXTURE_ARRAY(Texture2D, MaterialTextureTable, MaterialTextureTableFixedCapacity)
	SHADER_PARAMETER_SAMPLER(SamplerState, MaterialTextureSampler)
	SHADER_PARAMETER_CBUFFER_NAMED(RestirIndirectConstants, RestirIndirectLightingUniformData, RestirIndirectLightingUniformData)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    RestirIndirectTemporalCS,
    RendererShaderPackages::RestirIndirectTemporal,
    "Passes/RayTracing/RestirIndirectTemporal.hlsl",
    "main",
    Compute);
