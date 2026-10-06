#include "../../../PCH.h"
#include "Passes/Presentation/Display/OutputEncoding.h"

#include "Core/Public/Math/MathUtils.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraphTextureDesc.h"
#include "Passes/Presentation/Display/OutputEncodingCVars.h"
#include "RHI/Public/CVars/RHICVars.h"
#include "Passes/Presentation/Display/OutputEncodingShader.h"

SPARKLE_DEFINE_LOG_CATEGORY_STATIC(g_outputEncodingLogger, "Renderer.OutputEncoding");

static std::uint32_t ResolveOutputColorEncoding() noexcept
{
	switch (CVarOutputColorEncoding.Get())
	{
		case EngineOutputColorEncoding::Srgb:
			return 1u;
		case EngineOutputColorEncoding::Linear:
			return 0u;
		case EngineOutputColorEncoding::Automatic:
			break;
		default:
			Diagnostics::Fatal(g_outputEncodingLogger, __FILE__, __LINE__, "Output settings contain an unknown color encoding.");
	}

	const PixelFormat backBufferFormat = CVarBackBufferFormat.Get();
	const PixelFormat linearFormat = PixelFormatToLinear(backBufferFormat);
	if (linearFormat != PixelFormat::R8G8B8A8_UNorm && linearFormat != PixelFormat::B8G8R8A8_UNorm)
	{
		Diagnostics::Fatal(
		    g_outputEncodingLogger,
		    __FILE__,
		    __LINE__,
		    "Automatic output encoding received an unsupported back-buffer format.");
	}
	return IsSrgbPixelFormat(backBufferFormat) ? 0u : 1u;
}

FrameGraphTextureHandle AddOutputEncodingPass(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    FrameGraphTextureHandle displayLinearColor)
{
	const FrameGraphTextureHandle encodedColor = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "EncodedSceneColor",
	        settings.OutputExtent.Width,
	        settings.OutputExtent.Height,
	        PixelFormatToLinear(settings.OutputFormat)));

	auto& parameters = builder.AllocParameters<OutputEncodingCS>();
	parameters->DisplayLinearColor = builder.CreateSRV(displayLinearColor);
	parameters->EncodedColor = builder.CreateUAV(encodedColor);

	parameters->OutputColorEncoding = ResolveOutputColorEncoding();

	builder.Dispatch<OutputEncodingCS>(
	    parameters,
	    ComputeDispatchDesc{
	        MathUtils::DivideRoundUp(settings.OutputExtent.Width, 8u),
	        MathUtils::DivideRoundUp(settings.OutputExtent.Height, 8u),
	        1u});

	return encodedColor;
}
