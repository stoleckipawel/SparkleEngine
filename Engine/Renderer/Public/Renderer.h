#pragma once

#include "Renderer/Public/RendererAPI.h"

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
class Scene;
class Window;
class UI;
class TextureManager;
class FrameExecutor;
class PipelineStateManager;
class RendererWindowObserver;
class SceneRenderStateCoordinator;
class MaterialCacheManager;
class RenderSceneSnapshotCache;
class SceneViewBuilder;

class SPARKLE_RENDERER_API Renderer final
{
  public:
	Renderer(Timer& timer, Scene& scene, Window& window, LevelManager& levelManager) noexcept;
	~Renderer() noexcept;

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;
	Renderer(Renderer&&) = delete;
	Renderer& operator=(Renderer&&) = delete;

	void OnRender() noexcept;

  private:
	void PostLoad() noexcept;
	void InitializeCoreSystems(LevelManager& levelManager) noexcept;
	void InitializeSceneSystems(LevelManager& levelManager) noexcept;
	void InitializeFrameGraph() noexcept;
	void InitializeFrameExecutor() noexcept;
	void InitializeWindowObserver() noexcept;
	void RefreshFrameExecution() noexcept;

	Timer* m_timer = nullptr;
	Scene* m_scene = nullptr;
	Window* m_window = nullptr;

	std::unique_ptr<D3D12Rhi> m_rhi;

	std::unique_ptr<D3D12DescriptorHeapManager> m_descriptorHeapManager;
	std::unique_ptr<D3D12SwapChain> m_swapChain;
	std::unique_ptr<UI> m_ui;
	std::unique_ptr<D3D12FrameResourceManager> m_frameResourceManager;
	std::unique_ptr<D3D12ConstantBufferManager> m_constantBufferManager;
	std::unique_ptr<D3D12SamplerLibrary> m_samplerLibrary;
	std::unique_ptr<PipelineStateManager> m_pipelineStateManager;
	std::unique_ptr<GPUMeshCache> m_gpuMeshCache;
	std::unique_ptr<TextureManager> m_textureManager;
	std::unique_ptr<MaterialCacheManager> m_materialCacheManager;
	std::unique_ptr<SceneViewBuilder> m_sceneViewBuilder;
	std::unique_ptr<RenderSceneSnapshotCache> m_renderSceneSnapshotCache;
	std::unique_ptr<RenderCamera> m_renderCamera;
	std::unique_ptr<SceneRenderStateCoordinator> m_sceneRenderStateCoordinator;
	std::unique_ptr<FrameGraph> m_frameGraph;

	std::unique_ptr<FrameExecutor> m_frameExecutor;
	std::unique_ptr<RendererWindowObserver> m_windowObserver;
};
