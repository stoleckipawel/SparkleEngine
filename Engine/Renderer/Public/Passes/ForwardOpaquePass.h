#pragma once

#include "Renderer/Public/FrameGraph/RenderPass.h"
#include "Renderer/Public/FrameGraph/ResourceHandle.h"

class D3D12ConstantBufferManager;
class D3D12DepthStencil;
class D3D12DescriptorHeapManager;
class D3D12PipelineState;
class D3D12RootSignature;
class D3D12SamplerLibrary;
class D3D12SwapChain;
class GPUMeshCache;
class TextureManager;

class ForwardOpaquePass final : public RenderPass
{
  public:
	ForwardOpaquePass(
	    std::string_view name,
	    D3D12RootSignature& rootSignature,
	    D3D12PipelineState& pipelineState,
	    D3D12ConstantBufferManager& constantBufferManager,
	    D3D12DescriptorHeapManager& descriptorHeapManager,
	    TextureManager& textureManager,
	    D3D12SamplerLibrary& samplerLibrary,
	    GPUMeshCache& gpuMeshCache,
	    D3D12SwapChain& swapChain,
	    D3D12DepthStencil& depthStencil) noexcept;

	~ForwardOpaquePass() noexcept override = default;

	void Setup(PassBuilder& builder, const SceneView& sceneView) override;
	void Execute(RenderContext& context) override;

  private:
	void PrepareTargets(RenderContext& context);
	void ConfigurePipeline(RenderContext& context);
	void BindFrameResources(RenderContext& context);
	void BindGlobalResources(RenderContext& context);
	void DrawOpaqueMeshes(RenderContext& context);

	D3D12RootSignature* m_rootSignature = nullptr;
	D3D12PipelineState* m_pipelineState = nullptr;
	D3D12ConstantBufferManager* m_constantBufferManager = nullptr;
	D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;
	TextureManager* m_textureManager = nullptr;
	D3D12SamplerLibrary* m_samplerLibrary = nullptr;
	GPUMeshCache* m_gpuMeshCache = nullptr;
	D3D12SwapChain* m_swapChain = nullptr;
	D3D12DepthStencil* m_depthStencil = nullptr;

	const SceneView* m_sceneView = nullptr;
	ResourceHandle m_backBuffer;
	ResourceHandle m_depthBuffer;
};
