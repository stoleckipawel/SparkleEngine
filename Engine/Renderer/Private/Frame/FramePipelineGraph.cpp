#include "PCH.h"
#include "Frame/FramePipeline.h"

#include "Debug/RendererCVars.h"
#include "Frame/Graph/RenderFrameGraphFactory.h"
#include "Frame/RenderFrame.h"
#include "FrameGraph/FrameGraph.h"
#include "Providers/RendererImageProviderStack.h"
#include "Pipeline/RenderPassRuntimeCache.h"
#include "RayTracing/Effects/GBuffer/RayTracingGBufferExecutionPlan.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Presentation/RhiPresentationService.h"
#include "Resources/History/FrameHistory.h"
#include "Scene/RayTracing/RenderRayTracingScene.h"
#include "Scene/RenderScene.h"
#include "View/ViewportDisplaySettings.h"

RenderViewportExtent FramePipeline::ResolveOutputExtent() const noexcept
{
	if (m_viewportRenderRequest.Extent.IsValid())
	{
		return m_viewportRenderRequest.Extent;
	}

	return m_windowExtent;
}

RenderFrameGraphSettings FramePipeline::ResolveFrameGraphSettings() const noexcept
{
	const RenderViewportExtent outputExtent = ResolveOutputExtent();
	const LightingMode lighting = CVarLightingMode.Get();
	const ResolvedViewportDisplaySettings displaySettings = ResolvedViewportDisplaySettings::Resolve(m_viewportRenderRequest.Exposure);
	const ImageProviderPipeline imagePipeline = lighting == LightingMode::RestirPathTraced ? ImageProviderPipeline::RayReconstruction
	                                                                                       : ImageProviderPipeline::PresentationUpscaling;
	return RenderFrameGraphSettings{
	    .RenderExtent = m_imageProviders.ResolveRenderExtent(outputExtent, imagePipeline),
	    .OutputExtent = outputExtent,
	    .OutputFormat = m_deviceServices.GetRenderHardwareInterface().GetPresentationService().GetPresentColorFormat(),
	    .ExposureMeteringMethod = displaySettings.ExposureMeteringMethod,
	    .PresentationTarget = ShouldOutputToBackBuffer() ? FramePresentationTarget::BackBuffer : FramePresentationTarget::ViewportProduct,
	    .RequestedOutputs = m_viewportRenderRequest.RequestedOutputs};
}

bool FramePipeline::ShouldOutputToBackBuffer() const noexcept
{
	return m_viewportRenderRequest.ViewportId == 0;
}

void FramePipeline::InitializeFrameGraph() noexcept
{
	InitializeFrameGraph(ResolveFrameGraphSettings());
}

void FramePipeline::InitializeFrameGraph(const RenderFrameGraphSettings& settings) noexcept
{
	m_renderScene.GetRayTracingScene().GetShaderTablePlan().BeginMaterializationSet();
	const RenderFrameGraphDependencies dependencies{
	    .renderHardwareInterface = m_deviceServices.GetRenderHardwareInterface(),
	    .renderPassRuntimeCache = m_renderPassRuntimeCache,
	    .gpuMeshCache = m_gpuMeshCache,
	    .rayTracingScene = m_renderScene.GetRayTracingScene(),
	    .upscalerProvider = m_imageProviders.GetUpscalerProvider(),
	    .rayReconstructionProvider = m_imageProviders.GetRayReconstructionProvider(),
	    .window = m_window,
	    .settings = settings};

	RenderFrameGraphFactory frameGraphFactory(dependencies);
	RenderFrameGraphBuildResult buildResult = frameGraphFactory.Build();
	m_frameGraphSettings = settings;
	m_builtLightingMode = CVarLightingMode.Get();
	m_builtGBufferAlgorithm = CVarGBufferAlgorithm.Get();
	m_builtGBufferExecutionPlan = ResolveRayTracingGBufferExecutionPlan(m_renderScene.GetRayTracingScene().GetCapabilityReport());
	m_builtShadowExecutionPlan = m_builtLightingMode == LightingMode::RestirPathTraced
	    ? ResolveRayTracingShadowExecutionPlan(m_renderScene.GetRayTracingScene().GetCapabilityReport())
	    : RayTracingShadowExecutionPlan{};
	m_builtShaderTablePlanGeneration = m_renderScene.GetRayTracingScene().GetShaderTablePlan().GetGeneration();
	m_builtShaderGeneration = m_renderPassRuntimeCache.GetShaderGeneration();
	m_frameResources = buildResult.Resources;
	m_imageProviderFrameGraphKey = m_imageProviders.GetFrameGraphKey();
	m_frameGraph = std::move(buildResult.Graph);
	++m_graphTopologyGeneration;
}

void FramePipeline::RefreshFrameExecution(const RenderFrameGraphSettings& settings) noexcept
{
	RetireFrameExecution();
	InitializeFrameGraph(settings);
}

void FramePipeline::RebuildFrameExecutionAfterSwapChainDrain(const RenderFrameGraphSettings& settings) noexcept
{
	InitializeRenderFrames();
	m_frameGraph.reset();
	InitializeFrameGraph(settings);
}

void FramePipeline::RetireFrameExecution() noexcept
{
	if (m_frameGraph != nullptr)
	{
		m_frameExecutionRetirementQueue.Retire(m_deviceServices, std::move(m_frameGraph), std::move(m_renderFrames));
		InitializeRenderFrames();
	}
}

void FramePipeline::InvalidateViewHistory(RenderViewInvalidationReason reason) noexcept
{
	if (m_frameGraph != nullptr)
	{
		InvalidateFrameHistory(*m_frameGraph, m_frameResources.History);
	}
	m_renderViewState.Invalidate(reason);
	m_imageProviders.ResetHistory();
}

void FramePipeline::ApplyPendingResize() noexcept
{
	if (m_resizePending)
	{
		m_resizePending = false;
		InvalidateViewHistory(RenderViewInvalidationReason::GraphTopology);

		if (!m_windowMinimized && m_windowExtent.IsValid())
		{
			m_deviceServices.ResizeSwapChain();
			RebuildFrameExecutionAfterSwapChainDrain(ResolveFrameGraphSettings());
		}
	}
}

void FramePipeline::RefreshGraphForTopology() noexcept
{
	const ImageProviderGraphKey providerGraphKey = m_imageProviders.GetFrameGraphKey();
	const bool providerChanged = providerGraphKey != m_imageProviderFrameGraphKey;
	if (providerChanged)
	{
		m_imageProviders.Refresh();
	}

	const RenderFrameGraphSettings settings = ResolveFrameGraphSettings();
	const LightingMode lightingMode = CVarLightingMode.Get();
	const GBufferAlgorithm gBufferAlgorithm = CVarGBufferAlgorithm.Get();
	const RayTracingGBufferExecutionPlan gBufferExecutionPlan =
	    ResolveRayTracingGBufferExecutionPlan(m_renderScene.GetRayTracingScene().GetCapabilityReport());
	const RayTracingShadowExecutionPlan shadowExecutionPlan = lightingMode == LightingMode::RestirPathTraced
	    ? ResolveRayTracingShadowExecutionPlan(m_renderScene.GetRayTracingScene().GetCapabilityReport())
	    : RayTracingShadowExecutionPlan{};
	const std::uint64_t shaderTablePlanGeneration = m_renderScene.GetRayTracingScene().GetShaderTablePlan().GetGeneration();
	const std::uint64_t shaderGeneration = m_renderPassRuntimeCache.GetShaderGeneration();
	const bool usesSceneShaderTable = gBufferExecutionPlan.Active == RayTracingExecutionFrontend::Pipeline
	    || shadowExecutionPlan.Active == RayTracingExecutionFrontend::Pipeline
	    || m_builtGBufferExecutionPlan.Active == RayTracingExecutionFrontend::Pipeline
	    || m_builtShadowExecutionPlan.Active == RayTracingExecutionFrontend::Pipeline;
	if (providerChanged || settings != m_frameGraphSettings || lightingMode != m_builtLightingMode
	    || gBufferAlgorithm != m_builtGBufferAlgorithm
	    || gBufferExecutionPlan != m_builtGBufferExecutionPlan || shadowExecutionPlan != m_builtShadowExecutionPlan
	    || shaderGeneration != m_builtShaderGeneration
	    || (usesSceneShaderTable && shaderTablePlanGeneration != m_builtShaderTablePlanGeneration))
	{
		InvalidateViewHistory(RenderViewInvalidationReason::GraphTopology);
		RefreshFrameExecution(settings);
	}
}
