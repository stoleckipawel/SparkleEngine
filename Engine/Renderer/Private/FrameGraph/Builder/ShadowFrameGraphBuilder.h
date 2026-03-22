#pragma once

#include "RHI/Public/RenderConfig.h"
#include "Renderer/Public/FrameGraph/TextureHandle.h"

#include <array>
#include <cstddef>

class D3D12ConstantBufferManager;
class D3D12PipelineState;
class D3D12RootSignature;
class FrameGraph;

struct ShadowFrameGraphResources
{
	static constexpr std::size_t MaxShadowMaps = RenderConfig::Shadows::MaxShadowMaps;
	std::array<TextureHandle, MaxShadowMaps> shadowMapHandles = {};
};

class ShadowFrameGraphBuilder final
{
  public:
	ShadowFrameGraphBuilder(
	    D3D12RootSignature& rootSignature,
	    D3D12PipelineState& shadowPipelineState,
	    D3D12ConstantBufferManager& constantBufferManager) noexcept;

	ShadowFrameGraphResources Build(FrameGraph& frameGraph) const;

  private:
	D3D12RootSignature* m_rootSignature = nullptr;
	D3D12PipelineState* m_shadowPipelineState = nullptr;
	D3D12ConstantBufferManager* m_constantBufferManager = nullptr;
};