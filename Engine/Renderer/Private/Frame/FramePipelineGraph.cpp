#include "PCH.h"
#include "Frame/FramePipeline.h"

#include "Debug/RendererCVars.h"
#include "Frame/Graph/RenderFrameGraphFactory.h"
#include "Frame/RenderFrame.h"
#include "FrameGraph/FrameGraph.h"
#include "Providers/RendererImageProviderStack.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Presentation/RhiPresentationService.h"
#include "Resources/History/FrameHistory.h"
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
	const LightingMode lighting = GetLightingMode();
	const ResolvedViewportDisplaySettings displaySettings = ResolvedViewportDisplaySettings::Resolve(m_viewportRenderRequest.Exposure);
	const ImageProviderPipeline imagePipeline = lighting == LightingMode::RestirPathTraced ? ImageProviderPipeline::RayReconstruction
	                                                                                       : ImageProviderPipeline::PresentationUpscaling;
	return RenderFrameGraphSettings{
	    .RenderExtent = m_imageProviders.ResolveRenderExtent(outputExtent, imagePipeline),
	    .OutputExtent = outputExtent,
	    .OutputFormat = m_deviceServices.GetRenderHardwareInterface().GetPresentationService().GetPresentColorFormat(),
	    .ExposureMeteringMethod = displaySettings.ExposureMeteringMethod,
	    .PresentationTarget = ShouldOutputToBackBuffer() ? FramePresentationTarget::BackBuffer : FramePresentationTarget::ViewportProduct,
	    .GBuffer = CVarGBufferMode.Get(),
	    .Lighting = lighting,
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
	const RenderFrameGraphDependencies dependencies{
	    .renderHardwareInterface = m_deviceServices.GetRenderHardwareInterface(),
	    .renderPassRuntimeCache = m_renderPassRuntimeCache,
	    .gpuMeshCache = m_gpuMeshCache,
	    .rayTracingScene = m_renderScene.GetRayTracingSceneCapability(),
	    .upscalerProvider = m_imageProviders.GetUpscalerProvider(),
	    .rayReconstructionProvider = m_imageProviders.GetRayReconstructionProvider(),
	    .window = m_window,
	    .settings = settings};

	RenderFrameGraphFactory frameGraphFactory(dependencies);
	RenderFrameGraphBuildResult buildResult = frameGraphFactory.Build();
	m_frameGraphSettings = settings;
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
	if (providerChanged || settings != m_frameGraphSettings)
	{
		InvalidateViewHistory(RenderViewInvalidationReason::GraphTopology);
		RefreshFrameExecution(settings);
	}
}
