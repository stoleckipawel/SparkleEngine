#pragma once

#include <memory>

struct ComputePipelineStateDesc;
struct GraphicsPipelineStateDesc;
struct RenderBindingLayoutCompileDesc;
class D3D12Rhi;
class RenderBindingLayout;
class RenderPipelineState;

class D3D12PipelineService final
{
  public:
	explicit D3D12PipelineService(D3D12Rhi& rhi) noexcept;

	std::unique_ptr<RenderBindingLayout> CreateBindingLayout(const RenderBindingLayoutCompileDesc& desc);
	std::unique_ptr<RenderPipelineState> CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc);
	std::unique_ptr<RenderPipelineState> CreateComputePipelineState(const ComputePipelineStateDesc& desc);

  private:
	D3D12Rhi* m_rhi = nullptr;
};
