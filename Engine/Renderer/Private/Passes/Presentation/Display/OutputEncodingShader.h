#pragma once

#include "ShaderParameters/ShaderParameterStruct.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"

class OutputEncodingCS final : public GlobalShader<OutputEncodingCS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, OutputEncodingCS)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, EncodedColor)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, DisplayLinearColor)
	SHADER_PARAMETER(std::uint32_t, OutputColorEncoding)
	END_SHADER_PARAMETER_STRUCT()
};
