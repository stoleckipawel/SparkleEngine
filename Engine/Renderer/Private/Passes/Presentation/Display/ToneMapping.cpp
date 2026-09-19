#include "../../../PCH.h"
#include "Passes/Presentation/Display/ToneMapping.h"

#include "Core/Public/Math/MathUtils.h"
#include "Frame/Graph/RenderFrameGraphFormats.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraphTextureDesc.h"
#include "Passes/Presentation/Display/ToneMappingShader.h"
#include "Passes/Presentation/Display/ToneMappingSettings.h"
#include "View/RenderView.h"

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
	parameters->SceneColor = builder.CreateSRV(resources.Presentation.ResolvedSceneColor);
	parameters->ExposureTexture = builder.CreateSRV(resources.Transient.Exposure);
	parameters->ToneMappedColor = builder.CreateUAV(toneMappedColor);

	builder.AddParameterSetup<RenderView>(
	    parameters,
	    [](auto& fields, const RenderView& view)
	    { fields.ToneMappingConstants = BuildToneMappingUniformData(view.displaySettings.ToneMapper); });

	builder.Dispatch<ToneMappingCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(outputExtent.Width, 8u), MathUtils::DivideRoundUp(outputExtent.Height, 8u), 1u});

	return toneMappedColor;
}
