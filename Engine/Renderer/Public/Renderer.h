#pragma once

#include "RendererAPI.h"
#include "../../RHI/Public/Interop/ResourceState.h"
#include "Viewport/ViewportContracts.h"
#include "../../RHI/Public/Interop/RenderHardwareInterface.h"
#include "../../Core/Public/Events/ScopedEventHandle.h"

#include <cstdint>
#include <memory>
#include <vector>

class Timer;
class RendererBackendServices;

class FrameGraph;
class GPUMeshCache;
class LevelManager;
class RenderCamera;
class GameScene;
class Window;
class TextureManager;
class PipelineStateManager;
class SceneRenderStateCoordinator;
class MaterialCacheManager;
class PerViewDataBuilder;
class RenderSceneDataBuilder;
class ShadowBuilder;
class ShadowFrameBuilder;
class ViewLightingBuilder;
class FrameExecutionDiagnostics;
struct RenderSceneSnapshot;
struct ResolvedGpuTiming;

class SPARKLE_RENDERER_API Renderer final
{
  public:
	Renderer(Timer& timer, GameScene& gameScene, Window& window, LevelManager& levelManager) noexcept;
	~Renderer() noexcept;

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;
	Renderer(Renderer&&) = delete;
	Renderer& operator=(Renderer&&) = delete;

	void SubmitViewportRenderRequest(const ViewportRenderRequest& request) noexcept { m_viewportRenderRequest = request; }

	const ViewportRenderProducts& GetViewportRenderProducts() const noexcept { return m_viewportRenderProducts; }

	RenderHardwareInterface& GetRenderHardwareInterface() noexcept;
	const RenderHardwareInterface& GetRenderHardwareInterface() const noexcept;
	void ReloadCookedShaders() noexcept;
	std::uint64_t GetShaderPackageGeneration() const noexcept;
	void PrepareHostFrame() noexcept;
	void RecordHostFrame() noexcept;
	void SubmitHostFrame() noexcept;
	std::uint64_t ResolveRenderProductTextureId(RenderProductHandle handle) const noexcept;
	void TransitionRenderProduct(
	    NativeGraphicsCommandListHandle commandList,
	    RenderProductHandle handle,
	    ResourceState before,
	    ResourceState after) const noexcept;

	void OnRender() noexcept;

  private:
	void PostLoad() noexcept;
	void InitializeCoreSystems() noexcept;
	void InitializeSceneSystems(LevelManager& levelManager) noexcept;
	void InitializeFrameGraph() noexcept;
	void BindWindowResizeEvent() noexcept;
	void RefreshFrameExecution() noexcept;
	bool ShouldPresentSceneToBackBuffer() const noexcept;
	RenderViewportExtent ResolveSceneExtent() const noexcept;
	void BeginFrame() noexcept;
	void SetupFrame() noexcept;
	void RefreshViewportRenderProducts() noexcept;
	void RecordFrame() noexcept;
	void SubmitFrame() noexcept;
	void EndFrame() noexcept;
	FrameExecutionDiagnostics& GetCurrentFrameDiagnostics() noexcept;
	const FrameExecutionDiagnostics& GetCurrentFrameDiagnostics() const noexcept;
	void ReportResolvedTimings(std::uint32_t frameIndex, const FrameExecutionDiagnostics& frameDiagnostics) const noexcept;
	void PublishLiveGpuTimings(const std::vector<ResolvedGpuTiming>& resolvedTimers) const noexcept;

	Timer* m_timer = nullptr;
	GameScene* m_gameScene = nullptr;
	Window* m_window = nullptr;

	std::unique_ptr<RendererBackendServices> m_backend;
	std::unique_ptr<PipelineStateManager> m_pipelineStateManager;
	std::unique_ptr<GPUMeshCache> m_gpuMeshCache;
	std::unique_ptr<TextureManager> m_textureManager;
	std::unique_ptr<MaterialCacheManager> m_materialCacheManager;
	std::unique_ptr<RenderSceneDataBuilder> m_renderSceneDataBuilder;
	std::unique_ptr<PerViewDataBuilder> m_perViewDataBuilder;
	std::unique_ptr<ViewLightingBuilder> m_viewLightingBuilder;
	std::unique_ptr<ShadowBuilder> m_shadowBuilder;
	std::unique_ptr<ShadowFrameBuilder> m_shadowFrameBuilder;
	std::unique_ptr<RenderCamera> m_renderCamera;
	std::unique_ptr<SceneRenderStateCoordinator> m_sceneRenderStateCoordinator;
	std::unique_ptr<FrameGraph> m_frameGraph;
	std::vector<std::unique_ptr<FrameExecutionDiagnostics>> m_frameExecutionDiagnostics;
	std::unique_ptr<RenderSceneSnapshot> m_sceneSnapshot;
	RenderViewportExtent m_frameGraphSceneExtent = {};
	ViewportRenderRequest m_viewportRenderRequest = {};
	ViewportRenderProducts m_viewportRenderProducts = {};
	ScopedEventHandle m_resizeHandle;
	bool m_bResizePending = false;
};
