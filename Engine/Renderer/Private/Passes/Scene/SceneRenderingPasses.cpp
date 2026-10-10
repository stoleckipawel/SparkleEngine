#include "../../PCH.h"
#include "Passes/Scene/SceneRenderingPasses.h"

#include "Frame/FramePipeline.h"
#include "Frame/RenderFrame.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "View/RenderView.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Passes/Lighting/RealTimePathTracerPasses.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerPasses.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerSession.h"
#include "Passes/PostProcessing/Exposure/ExposurePasses.h"
#include "Passes/Presentation/Upscaling/SceneUpscalingPasses.h"
#include "Passes/Lighting/Restir/Reconstruction/RestirRayReconstruction.h"
#include "Passes/Scene/SceneVisualizationPasses.h"
#include "Passes/Presentation/PresentationPolicy.h"
#include "Scene/RenderScene.h"

std::uint64_t GetSceneRenderingGraphRebuildKey(RenderViewMode viewMode) noexcept
{
	constexpr std::uint32_t viewModeBitOffset = 8u;
	const std::uint64_t realTimePathTracerKey = viewMode == RenderViewMode::ReferencePathTracer ? 0u : GetRealTimePathTracerGraphRebuildKey();
	return (static_cast<std::uint64_t>(viewMode) << viewModeBitOffset) | realTimePathTracerKey;
}

void FramePipeline::AddSceneRenderingPasses(FrameGraphBuilder& builder, const RenderFrame& frame, RenderFrameGraphResources& resources)
{
	const RenderFrameGraphSettings& settings = m_frameGraphSettings;
	m_referencePathTracerSession->PublishFrameProducts(resources.ViewportProducts);
	if (frame.View.viewMode == RenderViewMode::ReferencePathTracer)
	{
		AddReferencePathTracerPasses(builder, frame, settings, *m_referencePathTracerSession, resources);
	}
	else
	{
		AddRealTimePathTracerPasses(builder, frame, settings, m_renderScene->GetRayTracingScene(), *m_gpuMeshCache, resources);
	}

	AddExposurePasses(builder, frame, settings, resources);
	AddSceneVisualizationPasses(builder, frame, settings.RenderExtent, resources);
	AddRestirRayReconstructionPass(builder, frame, settings.RenderExtent, *m_imageProviders, resources);
	AddSceneUpscalingPasses(builder, settings, ResolveSceneUpscalingMethod(frame.View.viewMode), *m_imageProviders, resources);
}
