#pragma once

#include "Renderer/Public/ShaderParameters/ShaderParameterStruct.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"

class LinearUpscaleCS final : public GlobalShader<LinearUpscaleCS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, ScalingOutputColor)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, ScalingInputColor)
	SHADER_PARAMETER_SHARED_SAMPLER(SamplerLinearClamp)
	END_SHADER_PARAMETER_STRUCT()
};
