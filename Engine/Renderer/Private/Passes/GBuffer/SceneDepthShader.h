#pragma once

#include "Renderer/Public/ShaderParameters/ShaderParameterStruct.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"
#include "ShaderData/ViewCameraUniformData.h"

class SceneDepthCS final : public GlobalShader<SceneDepthCS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferDeviceZ)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, SceneDepth)
	SHADER_PARAMETER_CBUFFER(ViewCameraUniformData, ViewCamera)
	END_SHADER_PARAMETER_STRUCT()
};
