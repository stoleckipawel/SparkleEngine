#pragma once

#include <memory>

#include "Renderer/Public/FrameGraph/TextureHandle.h"

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
};

struct FrameGraphBuildResult
{
	std::unique_ptr<FrameGraph> Graph;
	TextureHandle SceneColor;
	TextureHandle SceneDepth;
};

class FrameGraphBuilder final
{
  public:
	explicit FrameGraphBuilder(const FrameGraphDependencies& dependencies) noexcept;

	FrameGraphBuildResult Build() const;

  private:
	FrameGraphDependencies m_dependencies;
};