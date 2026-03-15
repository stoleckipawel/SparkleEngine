#pragma once

#include "Events/ScopedEventHandle.h"

class D3D12Rhi;
class GPUMeshCache;
class LevelChangeEvents;
class MaterialCacheManager;
class RenderCamera;
class RenderSceneSnapshotCache;
class Scene;
class TextureManager;

class SceneRenderStateCoordinator final
{
  public:
	SceneRenderStateCoordinator(
	    LevelChangeEvents& levelChangeEvents,
	    Scene& scene,
	    D3D12Rhi& rhi,
	    GPUMeshCache& gpuMeshCache,
	    TextureManager& textureManager,
	    RenderCamera& renderCamera,
	    RenderSceneSnapshotCache& renderSceneSnapshotCache,
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

	Scene* m_scene = nullptr;
	D3D12Rhi* m_rhi = nullptr;
	GPUMeshCache* m_gpuMeshCache = nullptr;
	TextureManager* m_textureManager = nullptr;
	RenderCamera* m_renderCamera = nullptr;
	RenderSceneSnapshotCache* m_renderSceneSnapshotCache = nullptr;
	MaterialCacheManager* m_materialCache = nullptr;
	ScopedEventHandle m_levelWillUnloadHandle;
	ScopedEventHandle m_levelChangedHandle;
};