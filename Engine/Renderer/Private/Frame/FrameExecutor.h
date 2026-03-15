#pragma once

class D3D12ConstantBufferManager;
class D3D12FrameResourceManager;
class D3D12Rhi;
class D3D12SwapChain;
class FrameGraph;
class RenderCamera;
class RenderSceneSnapshotCache;
class Scene;
class SceneViewBuilder;
class Timer;
class UI;
class Window;

class FrameExecutor final
{
  public:
	FrameExecutor(
	    Timer& timer,
	    Scene& scene,
	    Window& window,
	    D3D12Rhi& rhi,
	    D3D12SwapChain& swapChain,
	    D3D12FrameResourceManager& frameResourceManager,
	    RenderCamera& renderCamera,
	    UI& ui,
	    D3D12ConstantBufferManager& constantBufferManager,
	    FrameGraph& frameGraph,
	    RenderSceneSnapshotCache& renderSceneSnapshotCache,
	    SceneViewBuilder& sceneViewBuilder) noexcept;
	~FrameExecutor() noexcept = default;

	FrameExecutor(const FrameExecutor&) = delete;
	FrameExecutor& operator=(const FrameExecutor&) = delete;
	FrameExecutor(FrameExecutor&&) = delete;
	FrameExecutor& operator=(FrameExecutor&&) = delete;

	void ExecuteFrame() noexcept;

  private:
	void BeginFrame() noexcept;
	void SetupFrame() noexcept;
	void RecordFrame() noexcept;
	void SubmitFrame() noexcept;
	void EndFrame() noexcept;

	Timer* m_timer = nullptr;
	Scene* m_scene = nullptr;
	Window* m_window = nullptr;
	D3D12Rhi* m_rhi = nullptr;
	D3D12SwapChain* m_swapChain = nullptr;
	D3D12FrameResourceManager* m_frameResourceManager = nullptr;
	RenderCamera* m_renderCamera = nullptr;
	UI* m_ui = nullptr;
	D3D12ConstantBufferManager* m_constantBufferManager = nullptr;
	FrameGraph* m_frameGraph = nullptr;
	RenderSceneSnapshotCache* m_renderSceneSnapshotCache = nullptr;
	SceneViewBuilder* m_sceneViewBuilder = nullptr;
};