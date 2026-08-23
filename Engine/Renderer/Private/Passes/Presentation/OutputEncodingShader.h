#pragma once

#include "Renderer/Public/ShaderParameters/ShaderParameterStruct.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"
#include "Renderer/Private/ShaderData/OutputEncodingUniformData.h"

class OutputEncodingCS final : public GlobalShader<OutputEncodingCS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, EncodedColor)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, DisplayLinearColor)
	SHADER_PARAMETER_CBUFFER(OutputEncodingUniformData, OutputEncodingConstants)
	END_SHADER_PARAMETER_STRUCT()
};
