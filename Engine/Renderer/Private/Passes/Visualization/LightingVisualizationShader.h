#pragma once

#include "RHI/Public/Shaders/Authoring/GlobalShader.h"
#include "ShaderData/ViewUniformData.h"
#include "ShaderParameters/ShaderParameterStruct.h"

class LightingVisualizationCS final : public GlobalShader<LightingVisualizationCS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, SceneColor)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferBaseColor)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, DirectDiffuse)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, DirectSpecular)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, DirectSubsurface)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, IndirectDiffuse)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, IndirectSpecular)
	SHADER_PARAMETER_CBUFFER(ViewUniformData, View)
	END_SHADER_PARAMETER_STRUCT()
};
