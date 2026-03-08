// =============================================================================
// Renderer.h — High-Level Graphics Pipeline Orchestration
// =============================================================================
//
// Central rendering subsystem that manages the graphics pipeline, resource
// binding, and frame submission. Acts as the primary interface between game
// logic and the D3D12 RHI layer.
//
// USAGE:
//   Renderer renderer(scene);  // Construct with scene reference
//   renderer.OnRender();        // Call each frame from the main loop
//
// RESPONSIBILITIES:
//   - Pipeline state object (PSO) creation and management
//   - Per-frame and per-object constant buffer binding
//   - Depth buffer management and depth mode switching
//   - Scene traversal and mesh rendering
//   - Integration with UI overlay rendering
//
// NOTES:
//   - Owned by App, constructed after Window and before render loop
//   - Automatically subscribes to Window resize events
//   - Owns shader bytecode, textures, and pipeline objects
//
// =============================================================================

#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "Renderer/Public/SceneData/SceneView.h"

#include "Event.h"
#include "Events/ScopedEventHandle.h"
#include <cstdint>
#include <memory>
#include <vector>

enum class DepthMode : std::uint8_t;

class Timer;

// Forward declarations
class AssetSystem;
class D3D12Rhi;
class D3D12Texture;
class D3D12PipelineState;
class D3D12RootSignature;
class D3D12SamplerLibrary;
class D3D12DepthStencil;
class D3D12ConstantBufferManager;
class D3D12DescriptorHeapManager;
class D3D12FrameResourceManager;
class D3D12SwapChain;
class FrameGraph;
class GPUMeshCache;
class RenderCamera;
class Scene;
class Window;
class UI;
class TextureManager;
class ShaderCompileResult;
struct RendererMaterialCacheState;

// =============================================================================
// Renderer
// =============================================================================

class SPARKLE_RENDERER_API Renderer final
{
  public:
	// Constructs and initializes all rendering resources.
	Renderer(Timer& timer, const AssetSystem& assetSystem, Scene& scene, Window& window) noexcept;
	~Renderer() noexcept;

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;
	Renderer(Renderer&&) = delete;
	Renderer& operator=(Renderer&&) = delete;

	// =========================================================================
	// Frame Operations
	// =========================================================================

	// Executes a complete render frame: setup, scene traversal, UI, submission.
	void OnRender() noexcept;

  private:
	// -------------------------------------------------------------------------
	// Initialization Helpers
	// -------------------------------------------------------------------------

	void PostLoad() noexcept;
	void CreateDepthStencilBuffer();
	void CreatePSO();
	void OnDepthModeChanged(DepthMode mode) noexcept;
	void OnResize() noexcept;
	void SubscribeToDepthModeChanges() noexcept;
	void SubscribeToWindowResize() noexcept;

	// -------------------------------------------------------------------------
	// Frame Pipeline Stages
	// -------------------------------------------------------------------------

	void BeginFrame() noexcept;
	void SetupFrame() noexcept;
	void RecordFrame() noexcept;
	void SubmitFrame() noexcept;
	void EndFrame() noexcept;

	// -------------------------------------------------------------------------
	// Scene View (Cauldron-style frame data)
	// -------------------------------------------------------------------------

	/// Builds a SceneView from owned subsystems for the current frame.
	[[nodiscard]] SceneView BuildSceneView() const;

	/// Initializes viewport and camera references for the SceneView.
	void InitializeSceneView(SceneView& view) const;

	/// Populates materials from the scene's loaded material descriptions.
	void BuildMaterials(SceneView& view) const;
	void RebuildMaterialCache() const;
	void ReleaseMaterialTextureTables() const noexcept;

	/// Populates mesh draw commands from the scene's mesh list.
	void BuildMeshDraws(SceneView& view) const;

	// -------------------------------------------------------------------------
	// Owned Resources
	// -------------------------------------------------------------------------

	// Timer reference (not owned - owned by App)
	Timer* m_timer = nullptr;

	// AssetSystem reference (not owned - owned by App)
	const AssetSystem* m_assetSystem = nullptr;

	// RHI (OWNED - Renderer creates and manages the RHI)
	std::unique_ptr<D3D12Rhi> m_rhi;

	// GPU mesh cache for lazy uploading CPU meshes to GPU
	std::unique_ptr<GPUMeshCache> m_gpuMeshCache;

	// Texture manager
	std::unique_ptr<TextureManager> m_textureManager;

	// Frame buffers
	std::unique_ptr<D3D12DepthStencil> m_depthStencil;

	// Sampler library
	std::unique_ptr<D3D12SamplerLibrary> m_samplerLibrary;

	// Pipeline
	std::unique_ptr<D3D12PipelineState> m_pso;
	std::unique_ptr<D3D12RootSignature> m_rootSignature;

	// Compiled shaders (owned bytecode, hidden from public header)
	std::unique_ptr<ShaderCompileResult> m_vertexShader;
	std::unique_ptr<ShaderCompileResult> m_pixelShader;

	// Camera (set once at initialization)
	std::unique_ptr<RenderCamera> m_renderCamera;

	// Constant buffer manager
	std::unique_ptr<D3D12ConstantBufferManager> m_constantBufferManager;

	// Frame resource manager (per-frame GPU ring buffers)
	std::unique_ptr<D3D12FrameResourceManager> m_frameResourceManager;

	// Descriptor heap manager
	std::unique_ptr<D3D12DescriptorHeapManager> m_descriptorHeapManager;

	// Swap chain
	std::unique_ptr<D3D12SwapChain> m_swapChain;

	// UI (owned, created after descriptor heap manager)
	std::unique_ptr<UI> m_ui;

	// Frame Graph (owned, created after all dependencies)
	std::unique_ptr<FrameGraph> m_frameGraph;

	// Scene reference (not owned, for mesh access)
	Scene* m_scene = nullptr;

	// Window reference (not owned)
	Window* m_window = nullptr;

	// Event subscriptions (RAII - auto-cleanup on destruction)
	ScopedEventHandle m_depthModeChangedHandle;
	ScopedEventHandle m_resizeHandle;

	// Persistent material cache. Rebuilt only when the scene material set changes.
	mutable std::unique_ptr<RendererMaterialCacheState> m_materialCache;
};
