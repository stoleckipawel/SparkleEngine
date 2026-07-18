#include "PCH.h"
#include "Host/RendererSystemRoot.h"

#include "Camera/RenderCamera.h"
#include "Diagnostics/MeshDiagnosticsCollector.h"
#include "Diagnostics/RendererMemoryMonitor.h"
#include "Frame/Builders/PerViewDataBuilder.h"
#include "Frame/Builders/TemporalDataBuilder.h"
#include "Host/RendererBackendSystem.h"
#include "Level/LevelManager.h"
#include "Meshes/GPUMeshCache.h"
#include "Pipeline/PipelineStateManager.h"
#include "Providers/RendererImageProviderStack.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RayTracing/RayTracingCapabilityReport.h"
#include "RayTracing/Scene/RenderRayTracingScene.h"
#include "Renderer/Public/Debug/RendererCVars.h"
#include "RHI/Public/CVars/RHICVars.h"
#include "World/GameWorld.h"
#include "SceneData/Builders/RenderSceneDataBuilder.h"
#include "SceneData/Caching/MaterialCacheManager.h"
#include "SceneData/Lifecycle/SceneRenderStateCoordinator.h"
#include "Textures/TextureManager.h"
#include "Time/Timer.h"
#include "Window/Window.h"

RendererSystemRoot::RendererSystemRoot(Timer& timer, GameWorld& gameWorld, Window& window, LevelManager& levelManager) noexcept :
    m_timer(&timer), m_gameWorld(&gameWorld), m_window(&window)
{
	InitializeCoreSystems();
	InitializeSceneSystems(levelManager);
}

RendererSystemRoot::~RendererSystemRoot() noexcept
{
	if (m_backend != nullptr)
	{
		GetBackend().WaitForIdle();
	}
}

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
	if (m_gameWorld == nullptr)
	{
		return MeshDiagnosticsSnapshot{};
	}

	return MeshDiagnosticsCollector::Capture(m_gameWorld->GetMeshes(), m_gpuMeshCache.get());
}

TextureDiagnosticsSnapshot RendererSystemRoot::CaptureTextureDiagnostics() const
{
	return m_textureManager != nullptr ? m_textureManager->CaptureDiagnosticsSnapshot() : TextureDiagnosticsSnapshot{};
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

	GetBackend().WaitForIdle();
	m_imageProviders->Refresh(GetRenderHardwareInterface());
}

void RendererSystemRoot::InitializeCoreSystems() noexcept
{
	m_backend = std::make_unique<RendererBackendSystem>(*m_window, CVarBackBufferFormat.Get());
	RenderHardwareInterface& renderHardware = GetRenderHardwareInterface();
	{
		m_pipelineStateManager = std::make_unique<PipelineStateManager>(renderHardware);
	}
	{
		m_gpuMeshCache = std::make_unique<GPUMeshCache>(renderHardware);
	}

	RenderDiagnostics& backendDiagnostics = renderHardware.GetDiagnostics();
	const RayTracingCapabilityReport rayTracingCapabilities = RayTracingCapabilityReporter::Build(renderHardware.GetCapabilities());
	m_imageProviders = std::make_unique<RendererImageProviderStack>(renderHardware);
	m_renderRayTracingScene = std::make_unique<RenderRayTracingScene>(renderHardware, rayTracingCapabilities);

	m_memoryMonitor = std::make_unique<RendererMemoryMonitor>(backendDiagnostics);
}

void RendererSystemRoot::InitializeSceneSystems(LevelManager& levelManager) noexcept
{
	RenderHardwareInterface& renderHardware = GetRenderHardwareInterface();
	m_textureManager = std::make_unique<TextureManager>(
	    renderHardware.GetResourceService(),
	    renderHardware.GetDescriptorService(),
	    renderHardware.GetUploadService());
	m_materialCacheManager = std::make_unique<MaterialCacheManager>(*m_textureManager, GetRenderHardwareInterface());
	m_renderSceneDataBuilder = std::make_unique<RenderSceneDataBuilder>(*m_materialCacheManager, *m_gpuMeshCache, *m_textureManager);
	m_perViewDataBuilder = std::make_unique<PerViewDataBuilder>();
	m_temporalDataBuilder = std::make_unique<TemporalDataBuilder>();

	m_renderCamera = std::make_unique<RenderCamera>();

	m_sceneRenderStateCoordinator = std::make_unique<SceneRenderStateCoordinator>(
	    levelManager.GetLevelChangeEvents(),
	    *m_gameWorld,
	    GetBackend(),
	    *m_gpuMeshCache,
	    *m_textureManager,
	    *m_renderCamera,
	    *m_materialCacheManager,
	    *m_renderRayTracingScene);
}
