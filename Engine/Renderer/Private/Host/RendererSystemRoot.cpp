#include "PCH.h"
#include "Host/RendererSystemRoot.h"

#include "Camera/RenderCamera.h"
#include "Diagnostics/MeshDiagnosticsCollector.h"
#include "Diagnostics/RendererMemoryMonitor.h"
#include "Frame/Builders/PerViewDataBuilder.h"
#include "Frame/Builders/TemporalDataBuilder.h"
#include "Host/RendererBackendSystem.h"
#include "Host/RendererBackendConfiguration.h"
#include "Meshes/GPUMeshCache.h"
#include "Pipeline/PipelineStateManager.h"
#include "Providers/RendererImageProviderStack.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RayTracing/RayTracingCapabilityReport.h"
#include "RayTracing/Scene/RenderRayTracingScene.h"
#include "Renderer/Public/Debug/RendererCVars.h"
#include "RHI/Public/CVars/RHICVars.h"
#include "SceneData/RenderWorld.h"
#include "SceneData/Preparation/RenderPreparationGraph.h"
#include "SceneData/Caching/MaterialCacheManager.h"
#include "Textures/TextureManager.h"
#include "Window/Window.h"

#include <algorithm>
#include <array>

RendererSystemRoot::RendererSystemRoot(
    Window& window,
    const RendererBackendConfiguration& backendConfiguration,
    TaskExecutor& taskExecutor,
    TaskScope& applicationTaskScope) noexcept :
    m_window(&window),
    m_taskExecutor(&taskExecutor)
{
	InitializeCoreSystems(backendConfiguration);
	InitializeSceneSystems(taskExecutor, applicationTaskScope);
}

RendererSystemRoot::~RendererSystemRoot() noexcept = default;

RenderDeviceServices& RendererSystemRoot::GetBackend() noexcept
{
	return m_backend->GetServices();
}

const RenderDeviceServices& RendererSystemRoot::GetBackend() const noexcept
{
	return m_backend->GetServices();
}

RenderHardwareInterface& RendererSystemRoot::GetRenderHardwareInterface() noexcept
{
	return GetBackend().GetRenderHardwareInterface();
}

const RenderHardwareInterface& RendererSystemRoot::GetRenderHardwareInterface() const noexcept
{
	return GetBackend().GetRenderHardwareInterface();
}

RhiImGuiRenderer& RendererSystemRoot::GetImGuiRenderer() noexcept
{
	return GetBackend().GetImGuiRenderer();
}

CookedShaderReloadResult RendererSystemRoot::ReloadCookedShaders() noexcept
{
	if (m_pipelineStateManager != nullptr)
	{
		return m_pipelineStateManager->ReloadCookedShaders();
	}

	return CookedShaderReloadResult::Failure("Renderer has no pipeline state manager; cooked shader reload was skipped.");
}

std::uint64_t RendererSystemRoot::GetShaderPackageGeneration() const noexcept
{
	return m_pipelineStateManager != nullptr ? m_pipelineStateManager->GetShaderPackageGeneration() : 0;
}

MeshDiagnosticsSnapshot RendererSystemRoot::CaptureMeshDiagnostics() const
{
	return MeshDiagnosticsCollector::Capture(*m_renderWorld, m_gpuMeshCache.get());
}

TextureDiagnosticsSnapshot RendererSystemRoot::CaptureTextureDiagnostics(
    const TexturePreviewHandleResolver& resolvePreviewTexture) const
{
	return m_textureManager != nullptr
	           ? m_textureManager->CaptureDiagnosticsSnapshot(resolvePreviewTexture)
	           : TextureDiagnosticsSnapshot{};
}

RendererMemoryDiagnosticsSnapshot RendererSystemRoot::CaptureMemoryDiagnostics() const
{
	return m_memoryMonitor != nullptr ? m_memoryMonitor->GetLatestSnapshot() : RendererMemoryDiagnosticsSnapshot{};
}

void RendererSystemRoot::TickDiagnostics(std::uint64_t frameIndex) noexcept
{
	if (m_memoryMonitor != nullptr)
	{
		m_memoryMonitor->Tick(frameIndex);
	}
}

void RendererSystemRoot::PostLoad() noexcept
{
	GetBackend().CloseExecuteAndFlushCurrentFrame();
}

void RendererSystemRoot::RefreshImageProviders() noexcept
{
	if (m_backend == nullptr)
	{
		return;
	}

	RhiSubmissionState lastUse;
	for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
	{
		lastUse.MarkUsed(
		    GetBackend().GetLastSubmittedToken(
		        static_cast<ERhiQueueType>(queueIndex)));
	}
	m_retiredImageProviders.push_back(
	    RetiredImageProviderGeneration{
	        .LastUse = lastUse,
	        .Providers = std::move(m_imageProviders)});
	m_imageProviders =
	    std::make_unique<RendererImageProviderStack>(
	        GetRenderHardwareInterface());
}

void RendererSystemRoot::PollRetiredImageProviders() noexcept
{
	m_retiredImageProviders.erase(
	    std::remove_if(
	        m_retiredImageProviders.begin(),
	        m_retiredImageProviders.end(),
	        [this](const RetiredImageProviderGeneration& generation) noexcept
	        {
		        std::array<RhiSubmissionToken, RhiQueueTypeCount> tokens{};
		        const std::size_t tokenCount =
		            generation.LastUse.CopyTokens(tokens);
		        for (std::size_t tokenIndex = 0; tokenIndex < tokenCount; ++tokenIndex)
		        {
			        if (!GetBackend().IsSubmissionComplete(tokens[tokenIndex]))
			        {
				        return false;
			        }
		        }
		        return true;
	        }),
	    m_retiredImageProviders.end());
}

void RendererSystemRoot::InitializeCoreSystems(const RendererBackendConfiguration& backendConfiguration) noexcept
{
	m_backend = std::make_unique<RendererBackendSystem>(*m_window, CVarBackBufferFormat.Get(), backendConfiguration);
	RenderHardwareInterface& renderHardware = GetRenderHardwareInterface();
	{
		m_pipelineStateManager = std::make_unique<PipelineStateManager>(GetBackend());
	}
	{
		m_gpuMeshCache = std::make_unique<GPUMeshCache>(renderHardware);
	}

	RenderDiagnostics& backendDiagnostics = renderHardware.GetDiagnostics();
	const RayTracingCapabilityReport rayTracingCapabilities = RayTracingCapabilityReporter::Build(renderHardware.GetCapabilities());
	m_imageProviders = std::make_unique<RendererImageProviderStack>(renderHardware);
	m_renderRayTracingScene =
	    std::make_unique<RenderRayTracingScene>(
	        renderHardware,
	        *m_gpuMeshCache,
	        rayTracingCapabilities);

	m_memoryMonitor = std::make_unique<RendererMemoryMonitor>(backendDiagnostics);
}

void RendererSystemRoot::InitializeSceneSystems(
    TaskExecutor& taskExecutor,
    TaskScope& applicationTaskScope) noexcept
{
	RenderHardwareInterface& renderHardware = GetRenderHardwareInterface();
	m_textureManager = std::make_unique<TextureManager>(
	    renderHardware.GetResourceService(),
	    renderHardware.GetDescriptorService(),
	    renderHardware.GetUploadService(),
	    GetBackend(),
	    taskExecutor,
	    applicationTaskScope);
	m_materialCacheManager = std::make_unique<MaterialCacheManager>(*m_textureManager, GetRenderHardwareInterface());
	m_renderPreparationGraph =
	    std::make_unique<RenderPreparationGraph>(
	        taskExecutor,
	        *m_materialCacheManager,
	        *m_gpuMeshCache,
	        *m_textureManager);
	m_perViewDataBuilder = std::make_unique<PerViewDataBuilder>();
	m_temporalDataBuilder = std::make_unique<TemporalDataBuilder>();

	m_renderCamera = std::make_unique<RenderCamera>();
	m_renderWorld =
	    std::make_unique<RenderWorld>(&GetBackend());
}

MeshPreviewGeometry RendererSystemRoot::CaptureMeshPreview(std::uintptr_t meshRuntimeId) const
{
	return MeshDiagnosticsCollector::CapturePreview(*m_renderWorld, meshRuntimeId);
}
