#include "PCH.h"
#include "RendererShaderPackages.h"
#include "Renderer/Private/RayTracing/Effects/RestirLighting/RestirIndirectLightingUniformData.h"
#include "Renderer/Private/RayTracing/Effects/Shadows/RayTracedShadowUniformData.h"
#include "Renderer/Private/RayTracing/RayTracingHitData.h"
#include "Renderer/Private/RayTracing/RayTracingShaderFeatureFlags.h"
#include "Renderer/Private/Scene/Materials/MaterialTextureTableCapability.h"
#include "Shaders/Authoring/GlobalShader.h"
#include "ShaderData/FrameUniformData.h"
#include "ShaderData/ViewUniformData.h"
#include "ShaderData/ViewCameraUniformData.h"
#include "ShaderData/ViewTemporalUniformData.h"
#include "ShaderData/MeshInstanceShaderData.h"
#include "ShaderData/MorphTargetShaderData.h"
#include "ShaderData/LightGpuData.h"
#include "ShaderData/SceneLightingUniformData.h"
#include "ShaderData/SkyUniformData.h"

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
	SHADER_PARAMETER_CBUFFER_NAMED(Frame, FrameUniformData, FrameUniformData)
	SHADER_PARAMETER_CBUFFER_NAMED(View, ViewUniformData, ViewUniformData)
	SHADER_PARAMETER_CBUFFER_NAMED(ViewCamera, ViewCameraUniformData, ViewCameraUniformData)
	SHADER_PARAMETER_CBUFFER_NAMED(ViewTemporal, ViewTemporalUniformData, ViewTemporalUniformData)
	SHADER_PARAMETER_CBUFFER_NAMED(SceneLighting, SceneLighting, SceneLightingUniformData)
	SHADER_PARAMETER_CBUFFER_NAMED(RayTracedShadows, RayTracedShadowUniformData, RayTracedShadowUniformData)
	SHADER_PARAMETER_CBUFFER_NAMED(Sky, SkyUniformData, SkyUniformData)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferBaseColor)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferNormal)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferMaterial)
	SHADER_PARAMETER_TEXTURE(Texture2D, SceneDepth)
	SHADER_PARAMETER_TEXTURE(Texture2D, SkyTexture)
	SHADER_PARAMETER_SAMPLER(SamplerState, SamplerLinearClamp)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RayTracingHitVertex, RayTracingHitVertices)
	SHADER_PARAMETER_RDG_BUFFER_SRV(MorphTargetDeltaData, MorphTargetDeltas)
	SHADER_PARAMETER_RDG_BUFFER_SRV(uint32_t, RayTracingHitIndices)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RayTracingHitInstance, RayTracingHitInstances)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RayTracingHitMaterial, RayTracingHitMaterials)
	SHADER_PARAMETER_RDG_BUFFER_SRV(MeshInstanceData, MeshInstances)
	SHADER_PARAMETER_RDG_BUFFER_SRV(VertexSkinInfluenceData, SkinInfluences)
	SHADER_PARAMETER_RDG_BUFFER_SRV(JointMatrixData, JointMatrices)
	SHADER_PARAMETER_RDG_BUFFER_SRV(float, MorphWeights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(DirectionalLightGpuData, DirectionalLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(PointLightGpuData, PointLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(SpotLightGpuData, SpotLights)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RectLightGpuData, RectLights)
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
