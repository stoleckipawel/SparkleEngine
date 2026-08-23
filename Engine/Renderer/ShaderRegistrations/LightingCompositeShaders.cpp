#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStruct.h"

class LightingCompositeCS final : public GlobalShader<LightingCompositeCS>
{
  public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_TEXTURE_UAV_NAMED(RWTexture2D, SceneColor, SceneColorTexture)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, DirectDiffuse)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, DirectSpecular)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, DirectSubsurface)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, IndirectDiffuse)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, IndirectSpecular)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferBaseColor)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D, GBufferEmissive)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    LightingCompositeCS,
    RendererShaderPackages::LightingComposite,
    "/Engine/Passes/Lighting/LightingComposite.hlsl",
    "main",
    Compute);
