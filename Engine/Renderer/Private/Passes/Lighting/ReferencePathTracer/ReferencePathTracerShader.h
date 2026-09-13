#pragma once

#include "Renderer/Public/ShaderParameters/ShaderParameterStruct.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"
#include "ReferencePathTracerUniformData.h"
#include "Renderer/Private/RayTracing/RayTracingHitData.h"
#include "Renderer/Private/RayTracing/RayTracingShaderFeatureFlags.h"
#include "Renderer/Private/Scene/Materials/MaterialTextureTableCapability.h"
#include "ShaderData/LightGpuData.h"
#include "ShaderData/MeshInstanceShaderData.h"
#include "ShaderData/MorphTargetShaderData.h"
#include "ShaderData/RayTracingHitUniformData.h"
#include "ShaderData/SceneLightingUniformData.h"
#include "ShaderData/SkyUniformData.h"
#include "ShaderData/ViewCameraUniformData.h"

class ReferencePathTracerCS final : public GlobalShader<ReferencePathTracerCS>
{
public:
	static constexpr ShaderFeatureFlags kShaderFeatures = RayTracingShaderFeatureFlags::InlineRayQuery;

	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, WorkingMean)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, WorkingM2)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, CommittedMean)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, CommittedM2)
	SHADER_PARAMETER_ACCELERATION_STRUCTURE(SceneTlas)
	SHADER_PARAMETER_CBUFFER(ViewCameraUniformData, ViewCamera)
	SHADER_PARAMETER_CBUFFER(SkyUniformData, Sky)
	SHADER_PARAMETER_CBUFFER(SceneLightingUniformData, SceneLighting)
	SHADER_PARAMETER_CBUFFER(ReferencePathTracerUniformData, ReferencePathTracerConstants)
	SHADER_PARAMETER_CBUFFER(RayTracingHitUniformData, RayTracingHitConstants)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, SkyTexture)
	SHADER_PARAMETER_SHARED_SAMPLER(SamplerLinearWrapClamp)
	SHADER_PARAMETER_BUFFER_SRV(DirectionalLightGpuData, DirectionalLights)
	SHADER_PARAMETER_BUFFER_SRV(PointLightGpuData, PointLights)
	SHADER_PARAMETER_BUFFER_SRV(SpotLightGpuData, SpotLights)
	SHADER_PARAMETER_BUFFER_SRV(RectLightGpuData, RectLights)
	SHADER_PARAMETER_BUFFER_SRV(RayTracingHitVertex, RayTracingHitVertices)
	SHADER_PARAMETER_BUFFER_SRV(VertexSkinInfluenceData, SkinInfluences)
	SHADER_PARAMETER_BUFFER_SRV(MorphTargetDeltaData, MorphTargetDeltas)
	SHADER_PARAMETER_BUFFER_SRV(uint32_t, RayTracingHitIndices)
	SHADER_PARAMETER_BUFFER_SRV(RayTracingHitInstance, RayTracingHitInstances)
	SHADER_PARAMETER_BUFFER_SRV(RayTracingHitMaterial, RayTracingHitMaterials)
	SHADER_PARAMETER_BUFFER_SRV(MeshInstanceData, MeshInstances)
	SHADER_PARAMETER_BUFFER_SRV(JointMatrixData, JointMatrices)
	SHADER_PARAMETER_BUFFER_SRV(float, MorphWeights)
	SHADER_PARAMETER_TEXTURE_SRV_ARRAY(Texture2D, MaterialTextureTable, MaterialTextureTableFixedCapacity)
	END_SHADER_PARAMETER_STRUCT()
};

class ReferencePathTracerDisplayCS final : public GlobalShader<ReferencePathTracerDisplayCS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, WorkingMean)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, WorkingM2)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, CommittedMean)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, CommittedM2)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, SceneColor)
	SHADER_PARAMETER_CBUFFER(ReferencePathTracerUniformData, ReferencePathTracerConstants)
	END_SHADER_PARAMETER_STRUCT()
};
