#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "Events/ScopedEventHandle.h"

#include <memory>

class Timer;

class D3D12Rhi;
class D3D12PipelineState;
class D3D12RootSignature;
class D3D12SamplerLibrary;
class D3D12ConstantBufferManager;
class D3D12DescriptorHeapManager;
class D3D12FrameResourceManager;
class D3D12SwapChain;
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
struct RenderSceneSnapshot;

class SPARKLE_RENDERER_API Renderer final
{
  public:
	Renderer(
	    Timer& timer,
	    GameScene& gameScene,
	    Window& window,
	    LevelManager& levelManager) noexcept;
	~Renderer() noexcept;

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;
	Renderer(Renderer&&) = delete;
	Renderer& operator=(Renderer&&) = delete;

	void SubmitViewportRenderRequest(const ViewportRenderRequest& request) noexcept
	{
		m_viewportRenderRequest = request;
	}

	const ViewportRenderProducts& GetViewportRenderProducts() const noexcept
	{
		return m_viewportRenderProducts;
	}

	void OnRender() noexcept;

  private:
	void PostLoad() noexcept;
	void InitializeCoreSystems() noexcept;
	void InitializeSceneSystems(LevelManager& levelManager) noexcept;
	void InitializeFrameGraph() noexcept;
	void BindWindowResizeEvent() noexcept;
	void RefreshFrameExecution() noexcept;
	void BeginFrame() noexcept;
	void SetupFrame() noexcept;
	void RefreshViewportRenderProducts() noexcept;
	void RecordFrame() noexcept;
	void SubmitFrame() noexcept;
	void EndFrame() noexcept;

	Timer* m_timer = nullptr;
	GameScene* m_gameScene = nullptr;
	Window* m_window = nullptr;

	std::unique_ptr<D3D12Rhi> m_rhi;

	std::unique_ptr<D3D12DescriptorHeapManager> m_descriptorHeapManager;
	std::unique_ptr<D3D12SwapChain> m_swapChain;
	std::unique_ptr<D3D12FrameResourceManager> m_frameResourceManager;
	std::unique_ptr<D3D12ConstantBufferManager> m_constantBufferManager;
	std::unique_ptr<D3D12SamplerLibrary> m_samplerLibrary;
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
	std::unique_ptr<RenderSceneSnapshot> m_sceneSnapshot;
	ViewportRenderRequest m_viewportRenderRequest = {};
	ViewportRenderProducts m_viewportRenderProducts = {};
	ScopedEventHandle m_resizeHandle;
	bool m_bResizePending = false;
};
