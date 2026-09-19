#include "../../PCH.h"
#include "Passes/Presentation/OutputEncoding.h"

#include "Core/Public/Math/MathUtils.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraphTextureDesc.h"
#include "Passes/Presentation/OutputEncodingSettings.h"
#include "Passes/Presentation/OutputEncodingShader.h"

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
	builder.AddPassParameterSetup(
	    parameters,
	    [](auto& fields) { fields.OutputEncodingConstants = BuildOutputEncodingUniformData(); });
	builder.Dispatch<OutputEncodingCS>(
	    parameters,
	    ComputeDispatchDesc{
	        MathUtils::DivideRoundUp(settings.OutputExtent.Width, 8u),
	        MathUtils::DivideRoundUp(settings.OutputExtent.Height, 8u),
	        1u});
	return encodedColor;
}
