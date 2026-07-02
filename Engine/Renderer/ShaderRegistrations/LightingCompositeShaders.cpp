#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"

class LightingCompositeCS final : public TGlobalShader<LightingCompositeCS>
{
  public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, SceneColor, SceneColorTexture)
	SHADER_PARAMETER_TEXTURE(Texture2D, DirectDiffuse)
	SHADER_PARAMETER_TEXTURE(Texture2D, DirectSpecular)
	SHADER_PARAMETER_TEXTURE(Texture2D, DirectSubsurface)
	SHADER_PARAMETER_TEXTURE(Texture2D, IndirectDiffuse)
	SHADER_PARAMETER_TEXTURE(Texture2D, IndirectSpecular)
	SHADER_PARAMETER_TEXTURE(Texture2D, IndirectSubsurface)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferBaseColor)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferEmissive)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    LightingCompositeCS,
    RendererShaderPackages::LightingComposite,
    "Passes/Deferred/LightingComposite.hlsl",
    "main",
    Compute);
