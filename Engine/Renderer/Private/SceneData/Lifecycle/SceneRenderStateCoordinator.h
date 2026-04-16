#pragma once

#include "Events/ScopedEventHandle.h"

class GPUMeshCache;
class LevelChangeEvents;
class MaterialCacheManager;
class RenderCamera;
class RendererBackendServices;
class GameScene;
struct RenderSceneSnapshot;
class TextureManager;

class SceneRenderStateCoordinator final
{
  public:
	SceneRenderStateCoordinator(
	    LevelChangeEvents& levelChangeEvents,
	    GameScene& gameScene,
	    RendererBackendServices& backendServices,
	    GPUMeshCache& gpuMeshCache,
	    TextureManager& textureManager,
	    RenderSceneSnapshot& sceneSnapshot,
	    RenderCamera& renderCamera,
	    MaterialCacheManager& materialCache) noexcept;
	~SceneRenderStateCoordinator() noexcept = default;

	SceneRenderStateCoordinator(const SceneRenderStateCoordinator&) = delete;
	SceneRenderStateCoordinator& operator=(const SceneRenderStateCoordinator&) = delete;
	SceneRenderStateCoordinator(SceneRenderStateCoordinator&&) = delete;
	SceneRenderStateCoordinator& operator=(SceneRenderStateCoordinator&&) = delete;

  private:
	void SubscribeToLevelLifecycleEvents(LevelChangeEvents& levelChangeEvents) noexcept;
	void OnLevelWillUnload() noexcept;
	void OnLevelChanged() noexcept;
	void InvalidateSceneScopedRendererState() noexcept;
	void RefreshSceneScopedRendererState() noexcept;
	void ReleaseSceneScopedMaterialResources() noexcept;

	GameScene* m_gameScene = nullptr;
	RendererBackendServices* m_backendServices = nullptr;
	GPUMeshCache* m_gpuMeshCache = nullptr;
	TextureManager* m_textureManager = nullptr;
	RenderSceneSnapshot* m_sceneSnapshot = nullptr;
	RenderCamera* m_renderCamera = nullptr;
	MaterialCacheManager* m_materialCache = nullptr;
	ScopedEventHandle m_levelWillUnloadHandle;
	ScopedEventHandle m_levelChangedHandle;
};