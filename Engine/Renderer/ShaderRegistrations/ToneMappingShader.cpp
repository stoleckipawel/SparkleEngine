#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"

#include "Renderer/Private/Frame/Presentation/ToneMappingUniformData.h"

class ToneMappingCS final : public TGlobalShader<ToneMappingCS>
{
  public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV(RWTexture2D, ToneMappedColor)
	SHADER_PARAMETER_TEXTURE(Texture2D, SceneColor)
	SHADER_PARAMETER_TEXTURE(Texture2D, ExposureTexture)
	SHADER_PARAMETER_CBUFFER_NAMED(ToneMappingConstants, ToneMappingUniformData, ToneMappingUniformData)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    ToneMappingCS,
    RendererShaderPackages::ToneMapping,
    "Passes/Presentation/ToneMapping.hlsl",
    "main",
    Compute);
