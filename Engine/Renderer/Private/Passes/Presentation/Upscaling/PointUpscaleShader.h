#pragma once

#include "RHI/Public/Shaders/Authoring/GlobalShader.h"
#include "ShaderParameters/ShaderParameterStruct.h"

class PointUpscaleCS final : public GlobalShader<PointUpscaleCS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, ScalingInputColor)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, ScalingOutputColor)
	END_SHADER_PARAMETER_STRUCT()
};
