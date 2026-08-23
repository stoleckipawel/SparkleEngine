#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStruct.h"

#include "Renderer/Private/ShaderData/OutputEncodingUniformData.h"

class OutputEncodingCS final : public GlobalShader<OutputEncodingCS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, EncodedColor)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, DisplayLinearColor)
	SHADER_PARAMETER_CBUFFER_NAMED(OutputEncodingConstants, OutputEncodingUniformData, OutputEncodingUniformData)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    OutputEncodingCS,
    RendererShaderPackages::OutputEncoding,
    "/Engine/Passes/Presentation/OutputEncoding.hlsl",
    "main",
    Compute);
