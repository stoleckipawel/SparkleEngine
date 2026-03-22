#pragma once

#include "Renderer/Public/FrameGraph/TextureHandle.h"

#include <array>

class D3D12ConstantBufferManager;
class D3D12DescriptorHeapManager;
class D3D12PipelineState;
class D3D12RootSignature;
class D3D12SamplerLibrary;
class CommandContext;
class FrameGraph;
struct RenderSceneData;
struct RenderViewContext;
class TextureManager;

class ForwardOpaquePass final
{
  public:
	ForwardOpaquePass(
	    D3D12RootSignature& rootSignature,
	    D3D12PipelineState& pipelineState,
	    D3D12ConstantBufferManager& constantBufferManager,
	    D3D12DescriptorHeapManager& descriptorHeapManager,
	    TextureManager& textureManager,
	    D3D12SamplerLibrary& samplerLibrary,
	    std::array<TextureHandle, 2> shadowMapHandles,
	    TextureHandle backBufferHandle,
	    TextureHandle depthBufferHandle) noexcept;

	~ForwardOpaquePass() noexcept = default;

	void Execute(const FrameGraph& frameGraph, CommandContext& cmd, const RenderSceneData& sceneData, const RenderViewContext& viewContext);

  private:
	void PrepareTargets(const FrameGraph& frameGraph, CommandContext& cmd);
	void ConfigurePipeline(CommandContext& cmd, const RenderViewContext& viewContext);
	void BindFrameResources(CommandContext& cmd, const RenderViewContext& viewContext);
	void BindGlobalResources(const FrameGraph& frameGraph, CommandContext& cmd);
	void DrawOpaqueMeshes(CommandContext& cmd, const RenderSceneData& sceneData);

	D3D12RootSignature* m_rootSignature = nullptr;
	D3D12PipelineState* m_pipelineState = nullptr;
	D3D12ConstantBufferManager* m_constantBufferManager = nullptr;
	D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;
	TextureManager* m_textureManager = nullptr;
	D3D12SamplerLibrary* m_samplerLibrary = nullptr;
	std::array<TextureHandle, 2> m_shadowMaps = {};

	TextureHandle m_backBuffer;
	TextureHandle m_depthBuffer;
};
