#pragma once

#include "ShaderParameters/ShaderParameterStruct.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"

class ExposureHistogramClearCS final : public GlobalShader<ExposureHistogramClearCS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_BUFFER_UAV(uint32_t, HistogramCounts)
	END_SHADER_PARAMETER_STRUCT()
};

class ExposureHistogramBuildCS final : public GlobalShader<ExposureHistogramBuildCS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, SceneColor)
	SHADER_PARAMETER_BUFFER_UAV(uint32_t, HistogramCounts)
	END_SHADER_PARAMETER_STRUCT()
};

class ExposureHistogramResolveCS final : public GlobalShader<ExposureHistogramResolveCS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_BUFFER_SRV(uint32_t, HistogramCounts)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, LuminanceMomentsOutput)
	END_SHADER_PARAMETER_STRUCT()
};
