#include "../PCH.h"
#include "Frame/Upscaling.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Upscaling/UpscalerProvider.h"
#include "Upscaling/UpscalerSubsystem.h"

#include <cstdint>

namespace
{
	RenderProductHandle ToRenderProductHandle(FrameGraphTextureHandle handle) noexcept
	{
		if (!handle.IsValid())
		{
			return {};
		}

		return RenderProductHandle{static_cast<std::uint64_t>(handle.GetResourceHandle().index) + 1ull};
	}
}

void AddExternalProviderEvaluationPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer)
{
	builder.AddPass(
	    "EvaluateExternalUpscalerProvider",
	    EFrameGraphPassFlags::ExternalProvider,
	    [sceneTargets, gbuffer](PassResourceBuilder& resourceBuilder)
	    {
		    resourceBuilder.Read(sceneTargets.SceneColor, ResourceUsage::CopySource, "HudlessSceneColor");
		    resourceBuilder.Read(sceneTargets.MainDepth, ResourceUsage::DepthRead, "Depth");
		    resourceBuilder.Read(gbuffer.MotionVector, ResourceUsage::ShaderRead, "MotionVectors");
		    resourceBuilder.Write(sceneTargets.FinalSceneColor, ResourceUsage::CopyDest, "FinalSceneColor");
	    },
	    [sceneTargets, sceneExtent](PassExecutionContext& context)
	    {
		    UpscalerEvaluationResult result{
		        .ProducedOutput = false,
		        .UsedFallback = true,
		        .Reason = "No upscaler runtime service was provided."};

		    if (context.RuntimeServices.Upscaling != nullptr && context.RuntimeServices.Upscaling->Subsystem != nullptr)
		    {
			    result = context.RuntimeServices.Upscaling->Subsystem->Evaluate(
			        UpscalerEvaluationDesc{
			            .InputColor = ToRenderProductHandle(sceneTargets.SceneColor),
			            .OutputColor = ToRenderProductHandle(sceneTargets.FinalSceneColor),
			            .RenderExtent = sceneExtent,
			            .OutputExtent = sceneExtent});
		    }

		    if (!result.ProducedOutput || result.UsedFallback)
		    {
			    context.Resources.CopyTexture(context.Commands, sceneTargets.FinalSceneColor, sceneTargets.SceneColor);
		    }
	    });
}
