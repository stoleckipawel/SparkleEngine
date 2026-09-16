#include "PCH.h"
#include "Frame/FramePipeline.h"

#include "Debug/RendererCVars.h"
#include "Frame/RenderFrame.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraph.h"
#include "Providers/RendererImageProviderStack.h"
#include "Pipeline/RenderPassRuntimeCache.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Presentation/RhiPresentationService.h"
#include "Resources/History/FrameHistory.h"
#include "Scene/RayTracing/RenderRayTracingScene.h"
#include "Scene/RenderScene.h"
#include "View/ViewportDisplaySettings.h"

#include <string_view>

static void ExportTextureIfValid(FrameGraphBuilder& builder, FrameGraphTextureHandle handle, std::string_view name) noexcept
{
	if (handle.IsValid())
	{
		builder.ExportTexture(handle, name);
	}
}

static void ExportFrameProductRoots(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    const RenderFrameGraphResources& resources) noexcept
{
	ExportTextureIfValid(builder, resources.ViewportProducts.FinalSceneColor, "Viewport.FinalSceneColor");
	if (HasAnyRenderOutputFlags(settings.RequestedOutputs, RenderOutputFlags::SceneDepth))
	{
		ExportTextureIfValid(builder, resources.ViewportProducts.SceneDepth, "Viewport.SceneDepth");
	}
	if (HasAnyRenderOutputFlags(settings.RequestedOutputs, RenderOutputFlags::Normals))
	{
		ExportTextureIfValid(builder, resources.ViewportProducts.Normals, "Viewport.Normals");
	}
}

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
	const ResolvedViewportDisplaySettings displaySettings = ResolvedViewportDisplaySettings::Resolve(m_viewportRenderRequest.Exposure);
	const ImageProviderPipeline imagePipeline = m_viewportRenderRequest.ViewMode == RenderViewMode::ReferencePathTracer
	    ? ImageProviderPipeline::NativeResolution
	    : ImageProviderPipeline::RayReconstruction;
	return RenderFrameGraphSettings{
	    .RenderExtent = m_imageProviders->ResolveRenderExtent(outputExtent, imagePipeline),
	    .OutputExtent = outputExtent,
	    .ImagePipeline = imagePipeline,
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
	RenderRayTracingScene& rayTracingScene = m_renderScene->GetRayTracingScene();
	rayTracingScene.BeginGraphBuild();
	auto frameGraph = std::make_unique<FrameGraph>(&m_deviceServices.GetRenderHardwareInterface(), &m_window);
	FrameGraphBuilder builder(*frameGraph, m_renderPassRuntimeCache);
	RenderFrameGraphResources resources = BuildRenderFrameGraph(builder, settings);
	ExportFrameProductRoots(builder, settings, resources);

	m_frameGraphSettings = settings;
	m_builtGBufferAlgorithm = CVarGBufferAlgorithm.Get();
	m_builtRayTracingGraphGeneration = rayTracingScene.GetGraphGeneration();
	m_builtShaderGeneration = m_renderPassRuntimeCache.GetShaderGeneration();
	m_frameResources = resources;
	m_imageProviderFrameGraphKey = m_imageProviders->GetFrameGraphKey();
	m_frameGraph = std::move(frameGraph);
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
	m_renderViewState->Invalidate(reason);
	m_imageProviders->ResetHistory();
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
	const ImageProviderGraphKey providerGraphKey = m_imageProviders->GetFrameGraphKey();
	const bool providerChanged = providerGraphKey != m_imageProviderFrameGraphKey;
	if (providerChanged)
	{
		m_imageProviders->Refresh();
	}

	const RenderFrameGraphSettings settings = ResolveFrameGraphSettings();
	const GBufferAlgorithm gBufferAlgorithm = CVarGBufferAlgorithm.Get();
	const std::uint64_t rayTracingGraphGeneration = m_renderScene->GetRayTracingScene().GetGraphGeneration();
	const std::uint64_t shaderGeneration = m_renderPassRuntimeCache.GetShaderGeneration();
	if (providerChanged || settings != m_frameGraphSettings || gBufferAlgorithm != m_builtGBufferAlgorithm
	    || rayTracingGraphGeneration != m_builtRayTracingGraphGeneration || shaderGeneration != m_builtShaderGeneration)
	{
		InvalidateViewHistory(RenderViewInvalidationReason::GraphTopology);
		RefreshFrameExecution(settings);
	}
}
