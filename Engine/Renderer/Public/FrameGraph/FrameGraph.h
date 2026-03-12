// =============================================================================
// FrameGraph.h — Render Pass Graph Manager
// =============================================================================
//
// Manages render passes and their execution order. All passes declare their
// resource dependencies in Setup, then record GPU commands in Execute.
//
// USAGE:
//   SceneView view = renderer.BuildSceneView();
//   frameGraph.Setup(view);       // Passes declare resource usage
//   frameGraph.Compile();         // MVP: no-op
//   frameGraph.Execute(context);  // Passes record GPU commands
//
// DESIGN:
//   - Owns all render passes via unique_ptr
//   - Two-phase per frame: Setup (declare) then Execute (record)
//   - Future: automatic barrier insertion and pass reordering
//
// =============================================================================

#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "Renderer/Public/FrameGraph/RenderPass.h"
#include "Renderer/Public/FrameGraph/PassBuilder.h"

#include <memory>
#include <vector>
#include <utility>

// Forward declarations
class D3D12SwapChain;
class D3D12DepthStencil;
class RenderContext;
struct SceneView;

// =============================================================================
// FrameGraph
// =============================================================================

class SPARKLE_RENDERER_API FrameGraph
{
  public:
	FrameGraph(D3D12SwapChain* swapChain, D3D12DepthStencil* depthStencil);
	~FrameGraph();

	FrameGraph(const FrameGraph&) = delete;
	FrameGraph& operator=(const FrameGraph&) = delete;
	FrameGraph(FrameGraph&&) = delete;
	FrameGraph& operator=(FrameGraph&&) = delete;

	// -------------------------------------------------------------------------
	// Pass Registration
	// -------------------------------------------------------------------------

	/// Creates and registers a render pass of the given type.
	template <typename T, typename... Args> T& AddPass(std::string_view name, Args&&... args)
	{
		static_assert(std::is_base_of_v<RenderPass, T>, "T must derive from RenderPass");
		auto pass = std::make_unique<T>(name, std::forward<Args>(args)...);
		T& ref = *pass;
		m_passes.push_back(std::move(pass));
		return ref;
	}

	// -------------------------------------------------------------------------
	// Frame Execution
	// -------------------------------------------------------------------------

	/// Calls Setup() on each pass so they can declare resource usage.
	void Setup(const SceneView& sceneView);

	/// Compiles the frame graph. MVP: no-op.
	/// Future: dependency analysis, barrier insertion, pass reordering.
	void Compile();

	/// Calls Execute() on each pass to record GPU commands.
	void Execute(RenderContext& context);

	// -------------------------------------------------------------------------
	// Accessors
	// -------------------------------------------------------------------------

	std::size_t GetPassCount() const noexcept { return m_passes.size(); }
	D3D12SwapChain* GetSwapChain() const noexcept { return m_swapChain; }
	D3D12DepthStencil* GetDepthStencil() const noexcept { return m_depthStencil; }

  private:
	std::vector<std::unique_ptr<RenderPass>> m_passes;
	D3D12SwapChain* m_swapChain = nullptr;
	D3D12DepthStencil* m_depthStencil = nullptr;
	PassBuilder m_builder;
};
