#pragma once

#include "Renderer/Public/ShaderParameters/ShaderParameterStruct.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"
#include "Renderer/Private/ShaderData/ToneMappingUniformData.h"

class ToneMappingCS final : public GlobalShader<ToneMappingCS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, ToneMappedColor)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, SceneColor)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, ExposureTexture)
	SHADER_PARAMETER_CBUFFER(ToneMappingUniformData, ToneMappingConstants)
	END_SHADER_PARAMETER_STRUCT()
};
