#pragma once

#include "Events/ScopedEventHandle.h"

#include <string>

class GPUMeshCache;
class LevelChangeEvents;
class MaterialCacheManager;
class RenderCamera;
class RenderDeviceServices;
class GameScene;
class TextureManager;

class SceneRenderStateCoordinator final
{
  public:
	SceneRenderStateCoordinator(
	    LevelChangeEvents& levelChangeEvents,
	    GameScene& gameScene,
	    RenderDeviceServices& backendServices,
	    GPUMeshCache& gpuMeshCache,
	    TextureManager& textureManager,
	    RenderCamera& renderCamera,
	    MaterialCacheManager& materialCache) noexcept;
	~SceneRenderStateCoordinator() noexcept = default;

	SceneRenderStateCoordinator(const SceneRenderStateCoordinator&) = delete;
	SceneRenderStateCoordinator& operator=(const SceneRenderStateCoordinator&) = delete;
	SceneRenderStateCoordinator(SceneRenderStateCoordinator&&) = delete;
	SceneRenderStateCoordinator& operator=(SceneRenderStateCoordinator&&) = delete;

	bool ConsumeTemporalHistoryResetRequest(std::string& outReason) noexcept;

  private:
	void SubscribeToLevelLifecycleEvents(LevelChangeEvents& levelChangeEvents) noexcept;
	void OnLevelWillUnload() noexcept;
	void OnLevelChanged() noexcept;
	void InvalidateSceneScopedRendererState() noexcept;
	void RefreshSceneScopedRendererState() noexcept;
	void ReleaseSceneScopedMaterialResources() noexcept;

	GameScene* m_gameScene = nullptr;
	RenderDeviceServices* m_backendServices = nullptr;
	GPUMeshCache* m_gpuMeshCache = nullptr;
	TextureManager* m_textureManager = nullptr;
	RenderCamera* m_renderCamera = nullptr;
	MaterialCacheManager* m_materialCache = nullptr;
	ScopedEventHandle m_levelWillUnloadHandle;
	ScopedEventHandle m_levelChangedHandle;
	bool m_temporalHistoryResetRequested = false;
	std::string m_temporalHistoryResetReason;
};
