#pragma once

#include "Renderer/Public/ShaderParameters/ShaderParameterStruct.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"

class ExposureReduceSceneCS final : public GlobalShader<ExposureReduceSceneCS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, SceneColor)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, LuminanceMomentsOutput)
	END_SHADER_PARAMETER_STRUCT()
};
