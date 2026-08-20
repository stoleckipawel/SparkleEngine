#include "../../PCH.h"
#include "Frame/Presentation/Presentation.h"

#include "Frame/Core/FrameRenderFormats.h"
#include "Frame/Presentation/OutputEncodingSettings.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Builder/FrameGraphCopyPasses.h"
#include "Passes/Presentation/OutputEncodingPass.h"
#include "Passes/Presentation/ToneMappingPass.h"
#include "FrameGraph/FrameGraphTextureDesc.h"

class PresentationFormat final
{
public:
	static PixelFormat ResolveEncodedIntermediateFormat(PixelFormat backBufferFormat) noexcept
	{
		switch (backBufferFormat)
		{
			case PixelFormat::R8G8B8A8_UNorm_Srgb:
				return PixelFormat::R8G8B8A8_UNorm;
			case PixelFormat::B8G8R8A8_UNorm_Srgb:
				return PixelFormat::B8G8R8A8_UNorm;
			default:
				return backBufferFormat;
		}
	}
};

void AddPresentationPasses(FrameGraphBuilder& builder, const FrameBuildSettings& settings, FrameAssemblyResourceLayout& resources)
{
	const FrameGraphTextureHandle toneMappedColor = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "ToneMappedSceneColor",
	        settings.OutputExtent.Width,
	        settings.OutputExtent.Height,
	        FrameRenderFormats::SceneColor));
	const FrameGraphTextureHandle encodedColor = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "EncodedSceneColor",
	        settings.OutputExtent.Width,
	        settings.OutputExtent.Height,
	        PresentationFormat::ResolveEncodedIntermediateFormat(settings.OutputFormat)));

	auto& toneMappingParameters = builder.AllocParameters<ToneMappingPass::Parameters>();
	auto* toneMappingFields = toneMappingParameters.operator->();
	toneMappingParameters->SceneColor = builder.CreateSRV(resources.Transient.Scene.FinalSceneColor);
	toneMappingParameters->ExposureTexture = builder.CreateSRV(resources.Transient.Exposure);
	toneMappingParameters->ToneMappedColor = builder.CreateUAV(toneMappedColor);
	builder.AddToneMappingSetup(
	    [toneMappingFields](const ToneMappingUniformData& toneMapping) { toneMappingFields->ToneMappingConstants = toneMapping; });
	builder.Dispatch<ToneMappingPass>(toneMappingParameters, settings.OutputExtent.Width, settings.OutputExtent.Height);

	auto& outputEncodingParameters = builder.AllocParameters<OutputEncodingPass::Parameters>();
	auto* outputEncodingFields = outputEncodingParameters.operator->();
	outputEncodingParameters->DisplayLinearColor = builder.CreateSRV(toneMappedColor);
	outputEncodingParameters->EncodedColor = builder.CreateUAV(encodedColor);
	builder.AddPassParameterSetup(
	    [outputEncodingFields] { outputEncodingFields->OutputEncodingConstants = BuildOutputEncodingUniformData(); });
	builder.Dispatch<OutputEncodingPass>(outputEncodingParameters, settings.OutputExtent.Width, settings.OutputExtent.Height);
	if (settings.PresentationTarget == FramePresentationTarget::BackBuffer)
	{
		FrameGraphCopyPasses::AddTextureCopy(builder, "CopyEncodedColorToBackBuffer", resources.Transient.Scene.BackBuffer, encodedColor);
	}
	resources.ViewportProducts.FinalSceneColor = encodedColor;
}
