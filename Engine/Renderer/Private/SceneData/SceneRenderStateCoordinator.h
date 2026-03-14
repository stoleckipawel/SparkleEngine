#pragma once

#include "Events/ScopedEventHandle.h"

class D3D12Rhi;
class GPUMeshCache;
class LevelChangeEvents;
class RenderCamera;
class TextureManager;
struct RendererMaterialCacheState;

struct SceneRenderStateCoordinatorCallbacks
{
	void* context = nullptr;
	void (*releaseMaterialTextureTables)(void* context) noexcept = nullptr;
	void (*rebuildSceneResources)(void* context) noexcept = nullptr;
};

class SceneRenderStateCoordinator final
{
  public:
	SceneRenderStateCoordinator(
	    LevelChangeEvents& levelChangeEvents,
	    D3D12Rhi& rhi,
	    GPUMeshCache& gpuMeshCache,
	    TextureManager& textureManager,
	    RenderCamera& renderCamera,
	    RendererMaterialCacheState& materialCache,
	    SceneRenderStateCoordinatorCallbacks callbacks) noexcept;
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
	void ResetMaterialCacheState() noexcept;

	D3D12Rhi* m_rhi = nullptr;
	GPUMeshCache* m_gpuMeshCache = nullptr;
	TextureManager* m_textureManager = nullptr;
	RenderCamera* m_renderCamera = nullptr;
	RendererMaterialCacheState* m_materialCache = nullptr;
	SceneRenderStateCoordinatorCallbacks m_callbacks;
	ScopedEventHandle m_levelWillUnloadHandle;
	ScopedEventHandle m_levelChangedHandle;
};