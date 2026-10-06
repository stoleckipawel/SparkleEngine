#pragma once

#include "ShaderData/SkyUniformData.h"
#include "ShaderData/SceneLightingUniformData.h"
#include "ShaderData/ViewTemporalUniformData.h"
#include "ShaderData/ViewCameraUniformData.h"
#include "ShaderData/ViewUniformData.h"
#include "ShaderData/FrameUniformData.h"
#include "ShaderParameters/ShaderParameterStruct.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"
#include "Renderer/Private/RayTracing/RayTracingHitData.h"
#include "Renderer/Private/RayTracing/RayTracingShaderFeatureFlags.h"
#include "Renderer/Private/Scene/Materials/MaterialTextureTableCapability.h"
#include "ShaderData/MeshInstanceShaderData.h"
#include "ShaderData/MorphTargetShaderData.h"
#include "ShaderData/LightGpuData.h"

class RestirIndirectSpatialCS final : public GlobalShader<RestirIndirectSpatialCS>
{
public:
	static constexpr ShaderFeatureFlags kShaderFeatures = RayTracingShaderFeatureFlags::InlineRayQuery;

	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, RestirIndirectSpatialCS)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, TemporalReservoirSampleTexture)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, TemporalReservoirWeightTexture)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, CurrentReservoirSampleTexture)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, CurrentReservoirWeightTexture)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, CurrentReservoirSurfaceTexture)
	SHADER_PARAMETER_ACCELERATION_STRUCTURE(SceneTlas)
	SHADER_PARAMETER_CBUFFER(FrameUniformData, Frame)
	SHADER_PARAMETER_CBUFFER(ViewUniformData, View)
	SHADER_PARAMETER_CBUFFER(ViewCameraUniformData, ViewCamera)
	SHADER_PARAMETER_CBUFFER(ViewTemporalUniformData, ViewTemporal)
	SHADER_PARAMETER_CBUFFER(SceneLightingUniformData, SceneLighting)
	SHADER_PARAMETER(std::uint32_t, RayTracedDirectionalShadowsEnabled)
	SHADER_PARAMETER(std::uint32_t, RayTracedLocalLightShadowsEnabled)
	SHADER_PARAMETER(float, RayTracedShadowNormalBias)
	SHADER_PARAMETER(float, RayTracedShadowMaxDistance)
	SHADER_PARAMETER_CBUFFER(SkyUniformData, Sky)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferBaseColor)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferWorldNormal)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferMaterial)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, SceneDepth)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, SkyTexture)
	SHADER_PARAMETER_SAMPLER(SamplerState, SamplerLinearClamp)
	SHADER_PARAMETER_BUFFER_SRV(RayTracingHitVertex, RayTracingHitVertices)
	SHADER_PARAMETER_BUFFER_SRV(MorphTargetDeltaData, MorphTargetDeltas)
	SHADER_PARAMETER_BUFFER_SRV(uint32_t, RayTracingHitIndices)
	SHADER_PARAMETER_BUFFER_SRV(RayTracingHitInstance, RayTracingHitInstances)
	SHADER_PARAMETER(std::uint32_t, EmissiveEnabled)
	SHADER_PARAMETER_BUFFER_SRV(RayTracingHitMaterial, RayTracingHitMaterials)
	SHADER_PARAMETER_BUFFER_SRV(MeshInstanceData, MeshInstances)
	SHADER_PARAMETER_BUFFER_SRV(VertexSkinInfluenceData, SkinInfluences)
	SHADER_PARAMETER_BUFFER_SRV(JointMatrixData, JointMatrices)
	SHADER_PARAMETER_BUFFER_SRV(float, MorphWeights)
	SHADER_PARAMETER_BUFFER_SRV(DirectionalLightGpuData, DirectionalLights)
	SHADER_PARAMETER_BUFFER_SRV(PointLightGpuData, PointLights)
	SHADER_PARAMETER_BUFFER_SRV(SpotLightGpuData, SpotLights)
	SHADER_PARAMETER_BUFFER_SRV(RectLightGpuData, RectLights)
	SHADER_PARAMETER_TEXTURE_SRV_ARRAY(Texture2D, MaterialTextureTable, MaterialTextureTableFixedCapacity)
	SHADER_PARAMETER(std::uint32_t, RestirIndirectBounceCount)
	SHADER_PARAMETER(std::uint32_t, RestirIndirectTemporalReuse)
	SHADER_PARAMETER(std::uint32_t, RestirIndirectSpatialReuse)
	SHADER_PARAMETER(std::uint32_t, RestirIndirectEvaluateDiffuse)
	SHADER_PARAMETER(std::uint32_t, RestirIndirectEvaluateSpecular)
	SHADER_PARAMETER(std::uint32_t, RestirIndirectTraceSecondaryShadows)
	SHADER_PARAMETER(std::uint32_t, RestirIndirectWriteReconstructionGuides)
	END_SHADER_PARAMETER_STRUCT()
};
