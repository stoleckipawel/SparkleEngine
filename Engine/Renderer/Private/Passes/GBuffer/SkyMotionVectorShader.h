#pragma once

#include "Renderer/Public/ShaderParameters/ShaderParameterStruct.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"
#include "ShaderData/ViewUniformData.h"
#include "ShaderData/ViewCameraUniformData.h"
#include "ShaderData/ViewTemporalUniformData.h"

class SkyMotionVectorCS final : public GlobalShader<SkyMotionVectorCS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferDeviceZ)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, GBufferMotionVector)
	SHADER_PARAMETER_CBUFFER(ViewUniformData, View)
	SHADER_PARAMETER_CBUFFER(ViewCameraUniformData, ViewCamera)
	SHADER_PARAMETER_CBUFFER(ViewTemporalUniformData, ViewTemporal)
	END_SHADER_PARAMETER_STRUCT()
};
