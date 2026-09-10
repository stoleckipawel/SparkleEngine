#pragma once

#include "Renderer/Public/ShaderParameters/ShaderParameterStruct.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"
#include "Renderer/Private/RayTracing/Effects/ReferencePathTracer/ReferencePathTracerAccumulationUniformData.h"

class ReferencePathTracerAccumulationCS final : public GlobalShader<ReferencePathTracerAccumulationCS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, ReferencePathTracerSample)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, SceneColorTexture)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, PreviousReferencePathTracer)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, CurrentReferencePathTracer)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, ReferencePathTracerSampleValidity)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferMotionVector)
	SHADER_PARAMETER_CBUFFER(ReferencePathTracerAccumulationUniformData, ReferencePathTracerAccumulationConstants)
	END_SHADER_PARAMETER_STRUCT()
};
