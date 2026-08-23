#pragma once

#include "Renderer/Public/ShaderParameters/ShaderParameterStruct.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"
#include "ShaderData/ViewUniformData.h"

class VisualizeBuffersCS final : public GlobalShader<VisualizeBuffersCS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, SceneColor)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, DirectDiffuse)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, DirectSpecular)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, DirectSubsurface)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, IndirectDiffuse)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, IndirectSpecular)
	SHADER_PARAMETER_CBUFFER(ViewUniformData, View)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferBaseColor)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferNormal)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferMaterial)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferEmissive)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferSubsurface)
	END_SHADER_PARAMETER_STRUCT()
};
