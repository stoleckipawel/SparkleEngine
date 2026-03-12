#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "Renderer/Public/FrameGraph/RenderPass.h"
#include "Renderer/Public/FrameGraph/PassBuilder.h"

#include <memory>
#include <vector>
#include <utility>

class D3D12SwapChain;
class D3D12DepthStencil;
class RenderContext;
struct SceneView;

class SPARKLE_RENDERER_API FrameGraph
{
  public:
	FrameGraph(D3D12SwapChain* swapChain, D3D12DepthStencil* depthStencil);
	~FrameGraph();

	FrameGraph(const FrameGraph&) = delete;
	FrameGraph& operator=(const FrameGraph&) = delete;
	FrameGraph(FrameGraph&&) = delete;
	FrameGraph& operator=(FrameGraph&&) = delete;

	template <typename T, typename... Args> T& AddPass(std::string_view name, Args&&... args)
	{
		static_assert(std::is_base_of_v<RenderPass, T>, "T must derive from RenderPass");
		auto pass = std::make_unique<T>(name, std::forward<Args>(args)...);
		T& ref = *pass;
		m_passes.push_back(std::move(pass));
		return ref;
	}

	void Setup(const SceneView& sceneView);

	void Compile();

	void Execute(RenderContext& context);

	std::size_t GetPassCount() const noexcept { return m_passes.size(); }
	D3D12SwapChain* GetSwapChain() const noexcept { return m_swapChain; }
	D3D12DepthStencil* GetDepthStencil() const noexcept { return m_depthStencil; }

  private:
	std::vector<std::unique_ptr<RenderPass>> m_passes;
	D3D12SwapChain* m_swapChain = nullptr;
	D3D12DepthStencil* m_depthStencil = nullptr;
	PassBuilder m_builder;
};
