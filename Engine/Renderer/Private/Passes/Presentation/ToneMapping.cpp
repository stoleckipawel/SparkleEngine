#include "../../PCH.h"
#include "Passes/Presentation/ToneMapping.h"

#include "Core/Public/Math/MathUtils.h"
#include "Frame/Graph/RenderFrameGraphFormats.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraphTextureDesc.h"
#include "Passes/Presentation/ToneMappingShader.h"

FrameGraphTextureHandle AddToneMappingPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent outputExtent,
    const RenderFrameGraphResources& resources)
{
	const FrameGraphTextureHandle toneMappedColor = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "ToneMappedSceneColor",
	        outputExtent.Width,
	        outputExtent.Height,
	        RenderFrameGraphFormats::SceneColor));
	auto& parameters = builder.AllocParameters<ToneMappingCS>();
	parameters->SceneColor = builder.CreateSRV(resources.ResolvedSceneColor);
	parameters->ExposureTexture = builder.CreateSRV(resources.Transient.Exposure);
	parameters->ToneMappedColor = builder.CreateUAV(toneMappedColor);
	builder.AddParameterSetup<ToneMappingUniformData>(
	    parameters,
	    [](auto& fields, const ToneMappingUniformData& toneMapping) { fields.ToneMappingConstants = toneMapping; });
	builder.Dispatch<ToneMappingCS>(
	    parameters,
	    ComputeDispatchDesc{
	        MathUtils::DivideRoundUp(outputExtent.Width, 8u),
	        MathUtils::DivideRoundUp(outputExtent.Height, 8u),
	        1u});
	return toneMappedColor;
}
