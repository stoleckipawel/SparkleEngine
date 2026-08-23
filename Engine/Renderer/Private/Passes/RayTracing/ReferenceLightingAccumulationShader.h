#pragma once

#include "Renderer/Public/ShaderParameters/ShaderParameterStruct.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"
#include "Renderer/Private/RayTracing/Effects/ReferenceLighting/ReferenceLightingAccumulationUniformData.h"

class ReferenceLightingAccumulationCS final : public GlobalShader<ReferenceLightingAccumulationCS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, ReferenceLightingSample)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, SceneColorTexture)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, PreviousReferenceLighting)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, CurrentReferenceLighting)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, ReferenceSampleValidity)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferMotionVector)
	SHADER_PARAMETER_CBUFFER(ReferenceLightingAccumulationUniformData, ReferenceLightingAccumulationConstants)
	END_SHADER_PARAMETER_STRUCT()
};
