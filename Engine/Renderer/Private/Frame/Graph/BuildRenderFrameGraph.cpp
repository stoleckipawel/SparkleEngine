#include "../../PCH.h"
#include "Frame/FramePipeline.h"

#include "Debug/RendererCVars.h"
#include "Frame/Graph/RenderFrameGraphResourceBindings.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/GBuffer/GBuffer.h"
#include "Passes/Lighting/Lighting.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracer.h"
#include "Passes/PostProcessing/Exposure.h"
#include "Passes/PostProcessing/PostProcessing.h"
#include "Passes/Presentation/Upscaling.h"
#include "Passes/RayTracing/RayTracingScene.h"
#include "Providers/RendererImageProviderStack.h"
#include "Resources/History/FrameHistory.h"
#include "Scene/RenderScene.h"

RenderFrameGraphResources FramePipeline::BuildRenderFrameGraph(FrameGraphBuilder& builder, const RenderFrameGraphSettings& settings)
{
	RenderRayTracingScene& rayTracingScene = m_renderScene.GetRayTracingScene();
	RenderFrameGraphResources resources = {};
	CreateRenderFrameGraphResources(builder, settings, resources);
	AddRayTracingScenePasses(builder, rayTracingScene, resources);
	if (m_viewportRenderRequest.ViewMode == RenderViewMode::ReferencePathTracer)
	{
		m_referencePathTracer->AddPasses(builder, settings, resources);
	}
	else
	{
		DeclareRestirLightingHistoryResources(builder, settings.RenderExtent, resources.History);
		AddGBufferMeshPasses(builder, m_gpuMeshCache, rayTracingScene, settings.RenderExtent, resources);
		AddLightingPasses(builder, rayTracingScene, settings.RenderExtent, resources);
		AddExposurePass(builder, settings, resources);
		AddLightingReconstructionPasses(
		    builder,
		    settings.RenderExtent,
		    settings.OutputExtent,
		    m_imageProviders.GetRayReconstructionProvider(),
		    resources);
		if (!resources.ResolvedSceneColor.IsValid())
		{
			AddUpscalingPasses(builder, settings.RenderExtent, settings.OutputExtent, m_imageProviders.GetUpscalerProvider(), resources);
		}
	}
	AddPostProcessingPasses(builder, settings, resources);

	return resources;
}
