#pragma once

#include "ShaderData/FrameUniformData.h"
#include "ShaderParameters/ShaderParameterStruct.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"

class ExposureCS final : public GlobalShader<ExposureCS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, ExposureCS)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, ExposureTexture)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, ExposureHistoryTexture)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, PreviousExposureTexture)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, LuminanceMoments)
	SHADER_PARAMETER(std::uint32_t, ExposureMode)
	SHADER_PARAMETER(std::uint32_t, ExposureHistoryValid)
	SHADER_PARAMETER(float, ManualExposure)
	SHADER_PARAMETER(float, ExposureCompensation)
	SHADER_PARAMETER(float, ExposureTargetLuminance)
	SHADER_PARAMETER(float, ExposureMin)
	SHADER_PARAMETER(float, ExposureMax)
	SHADER_PARAMETER(float, ExposureAdaptationSpeedUp)
	SHADER_PARAMETER(float, ExposureAdaptationSpeedDown)
	SHADER_PARAMETER_CBUFFER(FrameUniformData, Frame)
	END_SHADER_PARAMETER_STRUCT()
};
