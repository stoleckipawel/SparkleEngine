#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"

#include "Renderer/Private/Frame/Presentation/OutputEncodingUniformData.h"

class OutputEncodingCS final : public TGlobalShader<OutputEncodingCS>
{
  public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV(RWTexture2D, EncodedColor)
	SHADER_PARAMETER_TEXTURE(Texture2D, DisplayLinearColor)
	SHADER_PARAMETER_SHARED_SAMPLER(SamplerLinearClamp)
	SHADER_PARAMETER_CBUFFER_NAMED(OutputEncodingConstants, OutputEncodingUniformData, OutputEncodingUniformData)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    OutputEncodingCS,
    RendererShaderPackages::OutputEncoding,
    "Passes/Presentation/OutputEncoding.hlsl",
    "main",
    Compute);
