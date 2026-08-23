#pragma once

#include "Renderer/Public/ShaderParameters/ShaderParameterStruct.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"

#include "ShaderData/MeshInstanceShaderData.h"
#include "ShaderData/MorphTargetShaderData.h"
#include "ShaderData/PerObjectConstantBufferData.h"
#include "ShaderData/ViewCameraUniformData.h"
#include "ShaderData/ViewTemporalUniformData.h"
#include "ShaderData/ViewUniformData.h"

class GBufferVS final : public GlobalShader<GBufferVS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_CBUFFER(ViewCameraUniformData, ViewCamera)
	SHADER_PARAMETER_CBUFFER(ViewTemporalUniformData, ViewTemporal)
	SHADER_PARAMETER_CBUFFER(MeshInstanceDrawConstantBufferData, MeshInstanceDraw)
	SHADER_PARAMETER_BUFFER_SRV(MeshInstanceData, MeshInstances)
	SHADER_PARAMETER_BUFFER_SRV(uint32_t, MeshInstanceSlots)
	SHADER_PARAMETER_EXTERNAL_BUFFER_SRV(VertexSkinInfluenceData, SkinInfluences)
	SHADER_PARAMETER_EXTERNAL_BUFFER_SRV(MorphTargetDeltaData, MorphTargetDeltas)
	SHADER_PARAMETER_BUFFER_SRV(JointMatrixData, JointMatrices)
	SHADER_PARAMETER_BUFFER_SRV(JointMatrixData, PreviousJointMatrices)
	SHADER_PARAMETER_BUFFER_SRV(float, MorphWeights)
	SHADER_PARAMETER_BUFFER_SRV(float, PreviousMorphWeights)
	END_SHADER_PARAMETER_STRUCT()
};

class GBufferPS final : public GlobalShader<GBufferPS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_CBUFFER(ViewUniformData, View)
	SHADER_PARAMETER_CBUFFER(PerObjectPSConstantBufferData, PerObjectPS)
	SHADER_PARAMETER_CBUFFER(ViewTemporalUniformData, ViewTemporal)
	SHADER_PARAMETER_EXTERNAL_TEXTURE_SRV(Texture2D, TextureBaseColor)
	SHADER_PARAMETER_EXTERNAL_TEXTURE_SRV(Texture2D, TextureNormal)
	SHADER_PARAMETER_EXTERNAL_TEXTURE_SRV(Texture2D, TextureRoughness)
	SHADER_PARAMETER_EXTERNAL_TEXTURE_SRV(Texture2D, TextureMetallic)
	SHADER_PARAMETER_EXTERNAL_TEXTURE_SRV(Texture2D, TextureOcclusion)
	SHADER_PARAMETER_EXTERNAL_TEXTURE_SRV(Texture2D, TextureEmissive)
	SHADER_PARAMETER_EXTERNAL_TEXTURE_SRV(Texture2D, TextureSubsurfaceColor)
	SHADER_PARAMETER_EXTERNAL_TEXTURE_SRV(Texture2D, TextureSubsurfaceStrength)
	SHADER_PARAMETER_SHARED_SAMPLER(SamplerAniso16xWrap)
	END_SHADER_PARAMETER_STRUCT()
};
