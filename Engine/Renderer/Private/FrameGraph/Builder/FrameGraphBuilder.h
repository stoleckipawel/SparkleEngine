#pragma once

#include <memory>

#include "Renderer/Public/FrameGraph/TextureHandle.h"

class D3D12ConstantBufferManager;
class D3D12DescriptorHeapManager;
class D3D12PipelineState;
class D3D12Rhi;
class D3D12RootSignature;
class D3D12SamplerLibrary;
class D3D12SwapChain;
class FrameGraph;
class GPUMeshCache;
class TextureManager;
class UI;
class Window;

struct FrameGraphDependencies
{
	D3D12Rhi& rhi;
	Window& window;
	D3D12RootSignature& rootSignature;
	D3D12PipelineState& pipelineState;
	D3D12ConstantBufferManager& constantBufferManager;
	TextureManager& textureManager;
	D3D12SamplerLibrary& samplerLibrary;
	GPUMeshCache& gpuMeshCache;
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