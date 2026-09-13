#pragma once

#include "Renderer/Public/ShaderParameters/ShaderParameterStruct.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"
#include "Renderer/Private/RayTracing/RayTracingHitData.h"
#include "Renderer/Private/RayTracing/RayTracingShaderFeatureFlags.h"
#include "ShaderData/MeshInstanceShaderData.h"
#include "ShaderData/ReferencePathTracerUniformData.h"
#include "ShaderData/RayTracingHitUniformData.h"
#include "ShaderData/SkyUniformData.h"
#include "ShaderData/ViewCameraUniformData.h"

class ReferencePathTracerCS final : public GlobalShader<ReferencePathTracerCS>
{
public:
	static constexpr ShaderFeatureFlags kShaderFeatures = RayTracingShaderFeatureFlags::InlineRayQueryCore;

	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, SceneColor)
	SHADER_PARAMETER_ACCELERATION_STRUCTURE(SceneTlas)
	SHADER_PARAMETER_CBUFFER(ViewCameraUniformData, ViewCamera)
	SHADER_PARAMETER_CBUFFER(SkyUniformData, Sky)
	SHADER_PARAMETER_CBUFFER(ReferencePathTracerUniformData, ReferencePathTracerConstants)
	SHADER_PARAMETER_CBUFFER(RayTracingHitUniformData, RayTracingHitConstants)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, SkyTexture)
	SHADER_PARAMETER_SHARED_SAMPLER(SamplerLinearClamp)
	SHADER_PARAMETER_BUFFER_SRV(RayTracingHitVertex, RayTracingHitVertices)
	SHADER_PARAMETER_BUFFER_SRV(uint32_t, RayTracingHitIndices)
	SHADER_PARAMETER_BUFFER_SRV(RayTracingHitInstance, RayTracingHitInstances)
	SHADER_PARAMETER_BUFFER_SRV(RayTracingHitMaterial, RayTracingHitMaterials)
	SHADER_PARAMETER_BUFFER_SRV(MeshInstanceData, MeshInstances)
	END_SHADER_PARAMETER_STRUCT()
};
