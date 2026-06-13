#pragma once

#include "Diagnostics/RendererMemoryDiagnostics.h"
#include "Meshes/MeshDiagnostics.h"
#include "Resources/Textures/TextureDiagnostics.h"
#include "Shaders/CookedShaderReloadResult.h"

#include <cstdint>
#include <memory>

class FrameExecutionDiagnostics;
class GameScene;
class GPUMeshCache;
class LevelManager;
class MaterialCacheManager;
class PerViewDataBuilder;
class PipelineStateManager;
class RenderCamera;
class RenderDeviceServices;
class RenderHardwareInterface;
class RendererMemoryMonitor;
class RenderRayTracingScene;
class RenderSceneDataBuilder;
class RenderSceneSnapshot;
class RhiImGuiRenderer;
class SceneRenderStateCoordinator;
class TemporalDataBuilder;
class TextureManager;
class Timer;
class UpscalerSubsystem;
class ViewLightingBuilder;
class Window;
struct RayTracedShadowSettings;

class RendererSystemRoot final
{
  public:
	RendererSystemRoot(Timer& timer, GameScene& gameScene, Window& window, LevelManager& levelManager) noexcept;
	~RendererSystemRoot() noexcept;

	RendererSystemRoot(const RendererSystemRoot&) = delete;
	RendererSystemRoot& operator=(const RendererSystemRoot&) = delete;
	RendererSystemRoot(RendererSystemRoot&&) = delete;
	RendererSystemRoot& operator=(RendererSystemRoot&&) = delete;

	Timer& GetTimer() noexcept { return *m_timer; }
	GameScene& GetGameScene() noexcept { return *m_gameScene; }
	const GameScene& GetGameScene() const noexcept { return *m_gameScene; }
	Window& GetWindow() noexcept { return *m_window; }
	const Window& GetWindow() const noexcept { return *m_window; }

	RenderDeviceServices& GetBackend() noexcept { return *m_backend; }
	const RenderDeviceServices& GetBackend() const noexcept { return *m_backend; }
	RenderHardwareInterface& GetRenderHardwareInterface() noexcept;
	const RenderHardwareInterface& GetRenderHardwareInterface() const noexcept;
	RhiImGuiRenderer& GetImGuiRenderer() noexcept;

	PipelineStateManager& GetPipelineStateManager() noexcept { return *m_pipelineStateManager; }
	GPUMeshCache& GetGpuMeshCache() noexcept { return *m_gpuMeshCache; }
	TextureManager& GetTextureManager() noexcept { return *m_textureManager; }
	MaterialCacheManager& GetMaterialCacheManager() noexcept { return *m_materialCacheManager; }
	RenderSceneDataBuilder& GetRenderSceneDataBuilder() noexcept { return *m_renderSceneDataBuilder; }
	RenderRayTracingScene* GetRenderRayTracingScene() noexcept { return m_renderRayTracingScene.get(); }
	const RenderRayTracingScene* GetRenderRayTracingScene() const noexcept { return m_renderRayTracingScene.get(); }
	PerViewDataBuilder& GetPerViewDataBuilder() noexcept { return *m_perViewDataBuilder; }
	TemporalDataBuilder& GetTemporalDataBuilder() noexcept { return *m_temporalDataBuilder; }
	ViewLightingBuilder& GetViewLightingBuilder() noexcept { return *m_viewLightingBuilder; }
	RenderCamera& GetRenderCamera() noexcept { return *m_renderCamera; }
	SceneRenderStateCoordinator* GetSceneRenderStateCoordinator() noexcept { return m_sceneRenderStateCoordinator.get(); }
	UpscalerSubsystem* GetUpscalerSubsystem() noexcept { return m_upscalerSubsystem.get(); }
	RenderSceneSnapshot& GetSceneSnapshot() noexcept { return *m_sceneSnapshot; }
	RayTracedShadowSettings* GetRayTracedShadowSettings() noexcept { return m_rayTracedShadowSettings.get(); }

	CookedShaderReloadResult ReloadCookedShaders() noexcept;
	std::uint64_t GetShaderPackageGeneration() const noexcept;
	MeshDiagnosticsSnapshot CaptureMeshDiagnostics() const;
	TextureDiagnosticsSnapshot CaptureTextureDiagnostics() const;
	RendererMemoryDiagnosticsSnapshot CaptureMemoryDiagnostics() const;
	void PostLoad() noexcept;

  private:
	void InitializeCoreSystems() noexcept;
	void InitializeSceneSystems(LevelManager& levelManager) noexcept;

	Timer* m_timer = nullptr;
	GameScene* m_gameScene = nullptr;
	Window* m_window = nullptr;

	std::unique_ptr<RenderDeviceServices> m_backend;
	std::unique_ptr<PipelineStateManager> m_pipelineStateManager;
	std::unique_ptr<RendererMemoryMonitor> m_memoryMonitor;
	std::unique_ptr<GPUMeshCache> m_gpuMeshCache;
	std::unique_ptr<TextureManager> m_textureManager;
	std::unique_ptr<MaterialCacheManager> m_materialCacheManager;
	std::unique_ptr<RenderSceneDataBuilder> m_renderSceneDataBuilder;
	std::unique_ptr<RenderRayTracingScene> m_renderRayTracingScene;
	std::unique_ptr<PerViewDataBuilder> m_perViewDataBuilder;
	std::unique_ptr<TemporalDataBuilder> m_temporalDataBuilder;
	std::unique_ptr<ViewLightingBuilder> m_viewLightingBuilder;
	std::unique_ptr<RenderCamera> m_renderCamera;
	std::unique_ptr<SceneRenderStateCoordinator> m_sceneRenderStateCoordinator;
	std::unique_ptr<UpscalerSubsystem> m_upscalerSubsystem;
	std::unique_ptr<RenderSceneSnapshot> m_sceneSnapshot;
	std::unique_ptr<RayTracedShadowSettings> m_rayTracedShadowSettings;
};
