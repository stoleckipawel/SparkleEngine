#pragma once

#include "RHI/Public/Shaders/Authoring/GlobalShader.h"
#include "ShaderData/ViewUniformData.h"
#include "ShaderParameters/ShaderParameterStruct.h"

class GBufferVisualizationCS final : public GlobalShader<GBufferVisualizationCS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, SceneColor)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferBaseColor)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferNormal)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferMaterial)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferEmissive)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferSubsurface)
	SHADER_PARAMETER_CBUFFER(ViewUniformData, View)
	END_SHADER_PARAMETER_STRUCT()
};
