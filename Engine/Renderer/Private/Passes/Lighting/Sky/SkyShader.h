#pragma once

#include "Renderer/Public/ShaderParameters/ShaderParameterStruct.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"
#include "ShaderData/ViewUniformData.h"
#include "ShaderData/ViewCameraUniformData.h"
#include "ShaderData/ViewTemporalUniformData.h"
#include "ShaderData/SkyUniformData.h"

class SkyCS final : public GlobalShader<SkyCS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, SceneColor)
	SHADER_PARAMETER_CBUFFER(ViewUniformData, View)
	SHADER_PARAMETER_CBUFFER(ViewCameraUniformData, ViewCamera)
	SHADER_PARAMETER_CBUFFER(ViewTemporalUniformData, ViewTemporal)
	SHADER_PARAMETER_CBUFFER(SkyUniformData, Sky)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, SceneDepth)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, SkyTexture)
	SHADER_PARAMETER_SHARED_SAMPLER(SamplerLinearClamp)
	END_SHADER_PARAMETER_STRUCT()
};
