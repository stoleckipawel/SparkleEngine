#pragma once

#include "Renderer/Public/FrameGraph/TextureHandle.h"

class D3D12ConstantBufferManager;
class D3D12PipelineState;
class D3D12RootSignature;
class CommandContext;
class FrameGraph;
struct RenderSceneData;
struct RenderViewContext;

class ShadowOpaquePass final
{
  public:
	ShadowOpaquePass(
	    D3D12RootSignature& rootSignature,
	    D3D12PipelineState& pipelineState,
	    D3D12ConstantBufferManager& constantBufferManager,
	    TextureHandle shadowMapHandle,
	    TextureHandle depthBufferHandle) noexcept;

	~ShadowOpaquePass() noexcept = default;

	void Execute(const FrameGraph& frameGraph, CommandContext& cmd, const RenderSceneData& sceneData, const RenderViewContext& viewContext);

  private:
	void PrepareTargets(const FrameGraph& frameGraph, CommandContext& cmd);
	void ConfigurePipeline(CommandContext& cmd, const RenderViewContext& viewContext);
	void BindFrameResources(CommandContext& cmd, const RenderViewContext& viewContext);
	void DrawMeshes(CommandContext& cmd, const RenderSceneData& sceneData);

	D3D12RootSignature* m_rootSignature = nullptr;
	D3D12PipelineState* m_pipelineState = nullptr;
	D3D12ConstantBufferManager* m_constantBufferManager = nullptr;
	TextureHandle m_shadowMap;
	TextureHandle m_depthBuffer;
};