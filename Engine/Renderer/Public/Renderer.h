#pragma once

#include "Renderer/Public/RendererAPI.h"
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
class UI;
class TextureManager;
class PipelineStateManager;
class SceneRenderStateCoordinator;
class MaterialCacheManager;
class RenderSceneViewBuilder;

class SPARKLE_RENDERER_API Renderer final
{
  public:
	Renderer(Timer& timer, GameScene& gameScene, Window& window, LevelManager& levelManager) noexcept;
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
	void BindWindowResizeEvent() noexcept;
	void RefreshFrameExecution() noexcept;
	void BeginFrame() noexcept;
	void SetupFrame() noexcept;
	void RecordFrame() noexcept;
	void SubmitFrame() noexcept;
	void EndFrame() noexcept;

	Timer* m_timer = nullptr;
	GameScene* m_gameScene = nullptr;
	Window* m_window = nullptr;

	std::unique_ptr<D3D12Rhi> m_rhi;

	std::unique_ptr<D3D12DescriptorHeapManager> m_descriptorHeapManager;
	std::unique_ptr<D3D12SwapChain> m_swapChain;
	std::unique_ptr<UI> m_editor;
	std::unique_ptr<D3D12FrameResourceManager> m_frameResourceManager;
	std::unique_ptr<D3D12ConstantBufferManager> m_constantBufferManager;
	std::unique_ptr<D3D12SamplerLibrary> m_samplerLibrary;
	std::unique_ptr<PipelineStateManager> m_pipelineStateManager;
	std::unique_ptr<GPUMeshCache> m_gpuMeshCache;
	std::unique_ptr<TextureManager> m_textureManager;
	std::unique_ptr<MaterialCacheManager> m_materialCacheManager;
	std::unique_ptr<RenderSceneViewBuilder> m_renderSceneViewBuilder;
	std::unique_ptr<RenderCamera> m_renderCamera;
	std::unique_ptr<SceneRenderStateCoordinator> m_sceneRenderStateCoordinator;
	std::unique_ptr<FrameGraph> m_frameGraph;
	ScopedEventHandle m_resizeHandle;
	bool m_bResizePending = false;
};
