#pragma once

#include "Renderer/Public/ShaderParameters/ShaderParameterStruct.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"
#include "Renderer/Private/ShaderData/ExposureUniformData.h"

class ExposureCS final : public GlobalShader<ExposureCS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, ExposureTexture)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, ExposureHistoryTexture)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, PreviousExposureTexture)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, LuminanceMoments)
	SHADER_PARAMETER_CBUFFER(ExposureUniformData, ExposureConstants)
	END_SHADER_PARAMETER_STRUCT()
};
