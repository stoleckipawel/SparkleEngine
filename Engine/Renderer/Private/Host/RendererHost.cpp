#include "PCH.h"
#include "Host/RendererHost.h"

#include "Core/Public/Diagnostics/Verify.h"
#include "Diagnostics/MeshDiagnosticsCollector.h"
#include "Diagnostics/RendererMemoryMonitor.h"
#include "Host/RendererBackendOwner.h"
#include "Host/RendererBackendConfiguration.h"
#include "Meshes/GpuMeshCache.h"
#include "Pipeline/RenderPassRuntimeCache.h"
#include "Providers/RendererImageProviderStack.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RayTracing/RayTracingCapabilityReport.h"
#include "Renderer/Public/Debug/RendererCVars.h"
#include "RHI/Public/CVars/RHICVars.h"
#include "Scene/RenderScene.h"
#include "Scene/Preparation/RenderScenePreparation.h"
#include "Textures/TextureCache.h"
#include "Window/Window.h"
#include "View/RenderViewBuilder.h"
#include "View/RenderViewPreparation.h"
#include "View/RenderViewState.h"

#include <algorithm>
#include <array>
#include <limits>

RendererHost::RendererHost(
    Window& window,
    const RendererBackendConfiguration& backendConfiguration,
    TaskExecutor& taskExecutor,
    TaskScope& applicationTaskScope) noexcept :
    m_window(&window),
    m_taskExecutor(&taskExecutor)
{
	InitializeCoreRuntime(backendConfiguration, taskExecutor, applicationTaskScope);
	InitializeSceneRuntime(taskExecutor, applicationTaskScope);
}

RendererHost::~RendererHost() noexcept = default;

RenderDeviceServices& RendererHost::GetDeviceServices() noexcept
{
	return m_backendOwner->GetDeviceServices();
}

const RenderDeviceServices& RendererHost::GetDeviceServices() const noexcept
{
	return m_backendOwner->GetDeviceServices();
}

RenderHardwareInterface& RendererHost::GetRenderHardwareInterface() noexcept
{
	return GetDeviceServices().GetRenderHardwareInterface();
}

const RenderHardwareInterface& RendererHost::GetRenderHardwareInterface() const noexcept
{
	return GetDeviceServices().GetRenderHardwareInterface();
}

RhiImGuiRenderer& RendererHost::GetImGuiRenderer() noexcept
{
	return GetDeviceServices().GetImGuiRenderer();
}

void RendererHost::ReloadCookedShaders()
{
	if (m_renderPassRuntimeCache == nullptr)
	{
		Diagnostics::Fatal(Logging::GetOrCreateLogger("Renderer.Host"), __FILE__, __LINE__, "Renderer has no render-pass runtime cache.");
	}

	m_renderPassRuntimeCache->ReloadCookedShaders();
}

std::uint64_t RendererHost::GetShaderPackageGeneration() const noexcept
{
	return m_renderPassRuntimeCache != nullptr ? m_renderPassRuntimeCache->GetShaderPackageGeneration() : 0;
}

MeshDiagnosticsSnapshot RendererHost::CaptureMeshDiagnostics() const
{
	return MeshDiagnosticsCollector::Capture(*m_renderScene, m_gpuMeshCache.get());
}

TextureDiagnosticsSnapshot RendererHost::CaptureTextureDiagnostics(const TexturePreviewHandleResolver& resolvePreviewTexture) const
{
	return m_textureCache != nullptr ? m_textureCache->CaptureDiagnosticsSnapshot(resolvePreviewTexture) : TextureDiagnosticsSnapshot{};
}

RendererMemoryDiagnosticsSnapshot RendererHost::CaptureMemoryDiagnostics() const
{
	return m_memoryMonitor != nullptr ? m_memoryMonitor->GetLatestSnapshot() : RendererMemoryDiagnosticsSnapshot{};
}

void RendererHost::TickDiagnostics(std::uint64_t frameIndex) noexcept
{
	if (m_memoryMonitor != nullptr)
	{
		m_memoryMonitor->Tick(frameIndex);
	}
}

void RendererHost::RefreshImageProviders() noexcept
{
	if (m_backendOwner == nullptr)
	{
		return;
	}
	if (m_imageProviderGeneration == (std::numeric_limits<std::uint64_t>::max)())
	{
		Diagnostics::Fatal(
		    Logging::GetOrCreateLogger("Renderer.Host"),
		    __FILE__,
		    __LINE__,
		    "Renderer image-provider generation exhausted.");
	}

	RhiSubmissionState lastUse;
	for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
	{
		lastUse.MarkUsed(GetDeviceServices().GetLastSubmittedToken(static_cast<ERhiQueueType>(queueIndex)));
	}
	m_retiredImageProviders.push_back(RetiredImageProviderGeneration{.LastUse = lastUse, .Providers = std::move(m_imageProviders)});
	m_imageProviders = std::make_unique<RendererImageProviderStack>(GetRenderHardwareInterface());
	++m_imageProviderGeneration;
}

void RendererHost::PollRetiredImageProviders() noexcept
{
	m_retiredImageProviders.erase(
	    std::remove_if(
	        m_retiredImageProviders.begin(),
	        m_retiredImageProviders.end(),
	        [this](const RetiredImageProviderGeneration& generation) noexcept
	        {
		        std::array<RhiSubmissionToken, RhiQueueTypeCount> tokens{};
		        const std::size_t tokenCount = generation.LastUse.CopyTokens(tokens);
		        for (std::size_t tokenIndex = 0; tokenIndex < tokenCount; ++tokenIndex)
		        {
			        if (!GetDeviceServices().IsSubmissionComplete(tokens[tokenIndex]))
			        {
				        return false;
			        }
		        }
		        return true;
	        }),
	    m_retiredImageProviders.end());
}

void RendererHost::InitializeCoreRuntime(
    const RendererBackendConfiguration& backendConfiguration,
    TaskExecutor& taskExecutor,
    TaskScope& applicationTaskScope) noexcept
{
	m_backendOwner = std::make_unique<RendererBackendOwner>(*m_window, CVarBackBufferFormat.Get(), backendConfiguration);
	RenderHardwareInterface& renderHardwareInterface = GetRenderHardwareInterface();
	{
		m_renderPassRuntimeCache = std::make_unique<RenderPassRuntimeCache>(GetDeviceServices());
	}
	{
		m_gpuMeshCache = std::make_unique<GpuMeshCache>(renderHardwareInterface, GetDeviceServices(), taskExecutor, applicationTaskScope);
	}

	RenderDiagnostics& backendDiagnostics = renderHardwareInterface.GetDiagnostics();
	m_imageProviders = std::make_unique<RendererImageProviderStack>(renderHardwareInterface);

	m_memoryMonitor = std::make_unique<RendererMemoryMonitor>(backendDiagnostics);
}

void RendererHost::InitializeSceneRuntime(TaskExecutor& taskExecutor, TaskScope& applicationTaskScope) noexcept
{
	RenderHardwareInterface& renderHardwareInterface = GetRenderHardwareInterface();
	const RayTracingCapabilityReport rayTracingCapabilities =
	    RayTracingCapabilityReporter::Build(renderHardwareInterface.GetCapabilities());
	m_textureCache = std::make_unique<TextureCache>(
	    renderHardwareInterface.GetResourceService(),
	    renderHardwareInterface.GetDescriptorService(),
	    renderHardwareInterface.GetUploadService(),
	    GetDeviceServices(),
	    taskExecutor,
	    applicationTaskScope);
	m_renderScenePreparation = std::make_unique<RenderScenePreparation>(taskExecutor, *m_gpuMeshCache, *m_textureCache);
	m_renderViewBuilder = std::make_unique<RenderViewBuilder>();
	m_renderViewPreparation = std::make_unique<RenderViewPreparation>(taskExecutor);
	m_renderViewState = std::make_unique<RenderViewState>();
	m_renderScene = std::make_unique<RenderScene>(
	    &GetDeviceServices(),
	    *m_gpuMeshCache,
	    *m_textureCache,
	    renderHardwareInterface,
	    rayTracingCapabilities);
}

MeshPreviewGeometry RendererHost::CaptureMeshPreview(std::uintptr_t meshRuntimeId) const
{
	return MeshDiagnosticsCollector::CapturePreview(*m_renderScene, meshRuntimeId);
}
