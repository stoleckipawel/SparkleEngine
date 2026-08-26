#pragma once

#include "Renderer/Public/ShaderParameters/ShaderParameterStruct.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"
#include "Renderer/Private/RayTracing/RayTracingHitData.h"
#include "Renderer/Private/RayTracing/RayTracingShaderFeatureFlags.h"
#include "Renderer/Private/Scene/Materials/MaterialTextureTableCapability.h"
#include "ShaderData/MeshInstanceShaderData.h"
#include "ShaderData/MorphTargetShaderData.h"
#include "ShaderData/RayTracingGBufferUniformData.h"
#include "ShaderData/ViewCameraUniformData.h"
#include "ShaderData/ViewTemporalUniformData.h"
#include "ShaderData/ViewUniformData.h"

class RayTracingGBufferRGS final : public GlobalShader<RayTracingGBufferRGS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
		SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, GBufferBaseColor)
		SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, GBufferNormal)
		SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, GBufferMaterial)
		SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, GBufferEmissive)
		SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, GBufferSubsurface)
		SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, GBufferDeviceZ)
		SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, GBufferMotionVector)
		SHADER_PARAMETER_ACCELERATION_STRUCTURE(SceneTlas)
		SHADER_PARAMETER_CBUFFER(ViewUniformData, View)
		SHADER_PARAMETER_CBUFFER(ViewCameraUniformData, ViewCamera)
		SHADER_PARAMETER_CBUFFER(ViewTemporalUniformData, ViewTemporal)
		SHADER_PARAMETER_CBUFFER(RayTracingGBufferUniformData, RayTracingGBufferConstants)
		SHADER_PARAMETER_BUFFER_SRV(RayTracingHitVertex, RayTracingHitVertices)
		SHADER_PARAMETER_BUFFER_SRV(MorphTargetDeltaData, MorphTargetDeltas)
		SHADER_PARAMETER_BUFFER_SRV(uint32_t, RayTracingHitIndices)
		SHADER_PARAMETER_BUFFER_SRV(RayTracingHitInstance, RayTracingHitInstances)
		SHADER_PARAMETER_BUFFER_SRV(RayTracingHitMaterial, RayTracingHitMaterials)
		SHADER_PARAMETER_BUFFER_SRV(MeshInstanceData, MeshInstances)
		SHADER_PARAMETER_BUFFER_SRV(VertexSkinInfluenceData, SkinInfluences)
		SHADER_PARAMETER_BUFFER_SRV(JointMatrixData, JointMatrices)
		SHADER_PARAMETER_BUFFER_SRV(JointMatrixData, PreviousJointMatrices)
		SHADER_PARAMETER_BUFFER_SRV(float, MorphWeights)
		SHADER_PARAMETER_BUFFER_SRV(float, PreviousMorphWeights)
		SHADER_PARAMETER_TEXTURE_SRV_ARRAY(Texture2D, MaterialTextureTable, MaterialTextureTableFixedCapacity)
		SHADER_PARAMETER_SAMPLER(SamplerState, MaterialTextureSampler)
	END_SHADER_PARAMETER_STRUCT()

	static constexpr ShaderFeatureFlags kShaderFeatures = RayTracingShaderFeatureFlags::SceneBindings;
	static constexpr RayTracingShaderMetadata kRayTracingMetadata{
	    .PayloadSizeInBytes = 24u,
	    .AttributeSizeInBytes = sizeof(float) * 2u,
	    .MinimumRecursionDepth = 1u};
};

class RayTracingGBufferInlineCS final : public GlobalShader<RayTracingGBufferInlineCS>
{
public:
	using Parameters = RayTracingGBufferRGS::Parameters;
	static constexpr ShaderFeatureFlags kShaderFeatures = RayTracingShaderFeatureFlags::InlineRayQuery;
};

class RayTracingGBufferMiss final : public GlobalShader<RayTracingGBufferMiss>
{
};

class RayTracingGBufferClosestHit final : public GlobalShader<RayTracingGBufferClosestHit>
{
};

class RayTracingGBufferAnyHit final : public GlobalShader<RayTracingGBufferAnyHit>
{
};
