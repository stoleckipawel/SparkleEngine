#pragma once

#include "RHI/Public/Shaders/Authoring/GlobalShader.h"
#include "ShaderParameters/ShaderParameterStruct.h"

class GpuSceneVisualizationCS final : public GlobalShader<GpuSceneVisualizationCS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, SceneColor)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferBaseColor)
	END_SHADER_PARAMETER_STRUCT()
};
