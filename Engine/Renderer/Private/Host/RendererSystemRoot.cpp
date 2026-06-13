#include "PCH.h"
#include "Host/RendererSystemRoot.h"

#include "Camera/RenderCamera.h"
#include "Config/RenderConfig.h"
#include "Core/Public/Diagnostics/Trace.h"
#include "Diagnostics/MeshDiagnosticsCollector.h"
#include "Diagnostics/RendererMemoryMonitor.h"
#include "Frame/Builders/PerViewDataBuilder.h"
#include "Frame/Builders/TemporalDataBuilder.h"
#include "Frame/Builders/ViewLightingBuilder.h"
#include "Level/LevelManager.h"
#include "Meshes/GPUMeshCache.h"
#include "Pipeline/PipelineStateManager.h"
#include "RHI/Public/Core/RhiBackendSelection.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RayTracing/RayTracedShadowSettings.h"
#include "RayTracing/RayTracingCapabilityReport.h"
#include "RayTracing/RenderRayTracingScene.h"
#include "Renderer/Public/Debug/RendererCVars.h"
#include "Scene/GameScene.h"
#include "SceneData/Builders/RenderSceneDataBuilder.h"
#include "SceneData/Caching/MaterialCacheManager.h"
#include "SceneData/Lifecycle/SceneRenderStateCoordinator.h"
#include "Textures/TextureManager.h"
#include "Time/Timer.h"
#include "Upscaling/UpscalerSubsystem.h"
#include "Upscaling/UpscalingStartupDiagnostics.h"
#include "Window/Window.h"

namespace
{
	bool UpgradePresentationInterfaceThroughRhi(
	    RhiNativeInterfaceUpgradeCallback callback,
	    void* callbackUserData,
	    void* bridgeUserData)
	{
		RenderHardwareInterface* const hardware = static_cast<RenderHardwareInterface*>(bridgeUserData);
		return hardware != nullptr && hardware->GetInteropService().UpgradePresentationInterface(callback, callbackUserData);
	}
}

RendererSystemRoot::RendererSystemRoot(Timer& timer, GameScene& gameScene, Window& window, LevelManager& levelManager) noexcept :
    m_timer(&timer), m_gameScene(&gameScene), m_window(&window)
{
	InitializeCoreSystems();
	InitializeSceneSystems(levelManager);
}

RendererSystemRoot::~RendererSystemRoot() noexcept
{
	if (m_upscalerSubsystem != nullptr)
	{
		m_upscalerSubsystem->Shutdown();
	}

	if (m_backend != nullptr)
	{
		m_backend->Flush();
	}
}

RenderHardwareInterface& RendererSystemRoot::GetRenderHardwareInterface() noexcept
{
	return m_backend->GetRenderHardwareInterface();
}

const RenderHardwareInterface& RendererSystemRoot::GetRenderHardwareInterface() const noexcept
{
	return m_backend->GetRenderHardwareInterface();
}

RhiImGuiRenderer& RendererSystemRoot::GetImGuiRenderer() noexcept
{
	return m_backend->GetImGuiRenderer();
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
	if (m_gameScene == nullptr)
	{
		return MeshDiagnosticsSnapshot{};
	}

	return MeshDiagnosticsCollector::Capture(m_gameScene->GetMeshes(), m_gpuMeshCache.get());
}

TextureDiagnosticsSnapshot RendererSystemRoot::CaptureTextureDiagnostics() const
{
	return m_textureManager != nullptr ? m_textureManager->CaptureDiagnosticsSnapshot() : TextureDiagnosticsSnapshot{};
}

RendererMemoryDiagnosticsSnapshot RendererSystemRoot::CaptureMemoryDiagnostics() const
{
	return m_memoryMonitor != nullptr ? m_memoryMonitor->GetLatestSnapshot() : RendererMemoryDiagnosticsSnapshot{};
}

void RendererSystemRoot::PostLoad() noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.PostLoad");
	m_backend->CloseExecuteAndFlushCurrentFrame();
}

void RendererSystemRoot::InitializeCoreSystems() noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.InitializeCoreSystems");

	{
		SPARKLE_CPU_SCOPE("Renderer.CreateBackend");
		m_backend = RenderDeviceServices::Create(*m_timer, *m_window);
	}
	{
		SPARKLE_CPU_SCOPE("Renderer.CreatePipelineStateManager");
		m_pipelineStateManager = std::make_unique<PipelineStateManager>(GetRenderHardwareInterface());
	}
	{
		SPARKLE_CPU_SCOPE("Renderer.CreateGPUMeshCache");
		m_gpuMeshCache = std::make_unique<GPUMeshCache>(GetRenderHardwareInterface());
	}

	RenderDiagnostics& backendDiagnostics = GetRenderHardwareInterface().GetDiagnosticsService().GetDiagnostics();
	const RayTracingCapabilityReport rayTracingCapabilities =
	    RayTracingCapabilityReporter::Build(GetRenderHardwareInterface().GetCapabilities());
	RayTracingCapabilityReporter::LogOnce(rayTracingCapabilities);
	LogUpscalingStartupDiagnostics(GetRenderHardwareInterface().GetCapabilities());
	m_upscalerSubsystem = std::make_unique<UpscalerSubsystem>();
	RenderHardwareInterface& renderHardware = GetRenderHardwareInterface();
	m_upscalerSubsystem->Initialize(
	    renderHardware.GetCapabilities(),
	    renderHardware.GetInteropService().GetDeviceQueueInterop(
	        RhiNativeInteropRequest{
	            .Consumer = ERhiNativeInteropConsumer::UpscalerProvider,
	            .Reason = "Renderer upscaler provider initialization"}),
	    UpscalerPresentationBridge{
	        .UpgradePresentationInterface = &UpgradePresentationInterfaceThroughRhi,
	        .UserData = &renderHardware});
	m_rayTracedShadowSettings = std::make_unique<RayTracedShadowSettings>(BuildRayTracedShadowSettingsFromCVars());
	LogRayTracedShadowSettingsOnce(*m_rayTracedShadowSettings, rayTracingCapabilities);
	m_renderRayTracingScene = std::make_unique<RenderRayTracingScene>(GetRenderHardwareInterface(), rayTracingCapabilities);

	m_memoryMonitor = std::make_unique<RendererMemoryMonitor>(backendDiagnostics);
}

void RendererSystemRoot::InitializeSceneSystems(LevelManager& levelManager) noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.InitializeSceneSystems");
	m_textureManager = std::make_unique<TextureManager>(GetRenderHardwareInterface());
	m_materialCacheManager = std::make_unique<MaterialCacheManager>(*m_textureManager, GetRenderHardwareInterface());
	m_renderSceneDataBuilder = std::make_unique<RenderSceneDataBuilder>(*m_materialCacheManager, *m_gpuMeshCache);
	m_perViewDataBuilder = std::make_unique<PerViewDataBuilder>();
	m_temporalDataBuilder = std::make_unique<TemporalDataBuilder>();
	m_viewLightingBuilder = std::make_unique<ViewLightingBuilder>();

	m_renderCamera = std::make_unique<RenderCamera>();

	m_sceneRenderStateCoordinator = std::make_unique<SceneRenderStateCoordinator>(
	    levelManager.GetLevelChangeEvents(),
	    *m_gameScene,
	    *m_backend,
	    *m_gpuMeshCache,
	    *m_textureManager,
	    *m_renderCamera,
	    *m_materialCacheManager);
}
