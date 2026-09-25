#include "../../../PCH.h"
#include "Passes/Presentation/Upscaling/SceneUpscalingPasses.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "Passes/Presentation/PresentationPolicy.h"
#include "Passes/Presentation/Upscaling/LinearUpscale.h"
#include "Passes/Presentation/Upscaling/PointUpscale.h"
#include "Upscaling/NvidiaDlss/NvidiaDlssUpscale.h"
#include "Upscaling/UpscalerPass.h"
#include "Upscaling/UpscalerSettings.h"

void AddSceneUpscalingPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RenderViewMode viewMode,
    RendererImageProviderStack& imageProviders,
    RenderFrameGraphResources& resources)
{
	const SceneUpscalingMethod method = ResolveSceneUpscalingMethod(viewMode);
	const EUpscalerProviderKind provider = CVarUpscalerProvider.Get();

	resources.Presentation.ResolvedSceneColor = CreateResolvedSceneColorTarget(builder, settings.OutputExtent);

	switch (method)
	{
		case SceneUpscalingMethod::Linear:
			AddLinearUpscalePass(
			    builder,
			    resources.Presentation.SceneColorInput,
			    resources.Presentation.ResolvedSceneColor,
			    settings.OutputExtent);

			return;
		case SceneUpscalingMethod::Point:
			AddPointUpscalePass(
			    builder,
			    resources.Presentation.SceneColorInput,
			    resources.Presentation.ResolvedSceneColor,
			    settings.OutputExtent);

			return;
		case SceneUpscalingMethod::ConfiguredProvider:
			break;
	}

	switch (provider)
	{
		case EUpscalerProviderKind::Linear:
			AddLinearUpscalePass(
			    builder,
			    resources.Presentation.SceneColorInput,
			    resources.Presentation.ResolvedSceneColor,
			    settings.OutputExtent);

			return;
		case EUpscalerProviderKind::NvidiaDlss:
		{
			const UpscalerPassResources inputs{
			    .InputColor = resources.Presentation.SceneColorInput,
			    .OutputColor = resources.Presentation.ResolvedSceneColor,
			    .Depth = resources.Transient.GBuffer.DeviceZ,
			    .MotionVectors = resources.Transient.GBuffer.MotionVector,
			    .Exposure = resources.Transient.Exposure};

			AddNvidiaDlssUpscalePass(builder, settings.RenderExtent, settings.OutputExtent, imageProviders, inputs);

			return;
		}
		default:
			throw Diagnostics::Error("Scene upscaling graph construction received an invalid provider.");
	}
}
