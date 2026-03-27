#pragma once

#include <memory>

#include "FrameGraph/TextureHandle.h"

class D3D12DescriptorHeapManager;
class D3D12Rhi;
class D3D12SwapChain;
class FrameGraph;
class UI;
class Window;

struct FrameGraphDependencies
{
	D3D12Rhi& rhi;
	Window& window;
	D3D12SwapChain& swapChain;
	D3D12DescriptorHeapManager& descriptorHeapManager;
	UI& ui;
};

class FrameGraphBuilder final
{
  public:
	explicit FrameGraphBuilder(const FrameGraphDependencies& dependencies) noexcept;

	std::unique_ptr<FrameGraph> Build() const;

  private:
	FrameGraphDependencies m_dependencies;
};