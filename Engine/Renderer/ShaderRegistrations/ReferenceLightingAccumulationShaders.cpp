#include "PCH.h"
#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStruct.h"

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
	SHADER_PARAMETER_CBUFFER_NAMED(
	    ReferenceLightingAccumulationConstants,
	    ReferenceLightingAccumulationUniformData,
	    ReferenceLightingAccumulationUniformData)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    ReferenceLightingAccumulationCS,
    RendererShaderPackages::ReferenceLightingAccumulation,
    "/Engine/Passes/RayTracing/ReferenceLightingAccumulation.hlsl",
    "main",
    Compute);
