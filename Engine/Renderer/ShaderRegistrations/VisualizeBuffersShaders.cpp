#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"

#include "Resources/RenderConstantBufferData.h"

#include <string_view>

void RegisterVisualizeBuffersShaders() noexcept {}

class VisualizeBuffersCS final : public TGlobalShader<VisualizeBuffersCS>
{
  public:
	static constexpr std::string_view kShaderName = "VisualizeBuffersCS";
	static constexpr std::string_view kShaderPackageName = RendererShaderPackages::VisualizeBuffers;
	static constexpr std::string_view kBindingLayoutId = RendererShaderPackages::VisualizeBuffers;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, SceneColor, SceneColorTexture)
	SHADER_PARAMETER_TEXTURE(Texture2D, DirectDiffuse)
	SHADER_PARAMETER_TEXTURE(Texture2D, DirectSpecular)
	SHADER_PARAMETER_TEXTURE(Texture2D, DirectSubsurface)
	SHADER_PARAMETER_TEXTURE(Texture2D, IndirectDiffuse)
	SHADER_PARAMETER_TEXTURE(Texture2D, IndirectSpecular)
	SHADER_PARAMETER_TEXTURE(Texture2D, IndirectSubsurface)
	SHADER_PARAMETER_CBUFFER_NAMED(PerFrame, PerFrameConstantBufferData, PerFrameConstantBufferData)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferBaseColor)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferNormal)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferMaterial)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferEmissive)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferSubsurface)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(VisualizeBuffersCS, "Passes/Deferred/VisualizeBuffers.hlsl", "main", Compute);
