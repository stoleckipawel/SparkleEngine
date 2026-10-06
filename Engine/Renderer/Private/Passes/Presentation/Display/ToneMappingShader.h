#pragma once

#include "ShaderParameters/ShaderParameterStruct.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"

class ToneMappingCS final : public GlobalShader<ToneMappingCS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, ToneMappingCS)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, ToneMappedColor)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, SceneColor)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, ExposureTexture)
	SHADER_PARAMETER(std::uint32_t, ToneMapper)
	END_SHADER_PARAMETER_STRUCT()
};
