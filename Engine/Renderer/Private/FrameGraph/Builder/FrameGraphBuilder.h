#pragma once

#include <memory>

#include "FrameGraph/TextureHandle.h"
#include "Renderer/Public/Overlays/RendererOverlay.h"

class D3D12DescriptorHeapManager;
class D3D12Rhi;
class D3D12SwapChain;
class FrameGraph;
class Window;

struct FrameGraphDependencies
{
	D3D12Rhi& rhi;
	Window& window;
	D3D12SwapChain& swapChain;
	D3D12DescriptorHeapManager& descriptorHeapManager;
	IRendererOverlay* overlay = nullptr;
};

class FrameGraphBuilder final
{
  public:
	explicit FrameGraphBuilder(const FrameGraphDependencies& dependencies) noexcept;

	std::unique_ptr<FrameGraph> Build() const;

  private:
	FrameGraphDependencies m_dependencies;
};