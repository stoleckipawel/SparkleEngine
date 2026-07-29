#pragma once

#include "Pipeline/RhiPipelineService.h"

#include <memory>

struct ComputePipelineDesc;
struct GraphicsPipelineDesc;
struct RenderBindingLayoutCompileDesc;
class D3D12Rhi;
class RenderBindingLayout;
class RenderPipeline;

class D3D12PipelineService final : public RhiPipelineService
{
  public:
	explicit D3D12PipelineService(D3D12Rhi& rhi) noexcept;

	std::unique_ptr<RenderBindingLayout> CreateBindingLayout(const RenderBindingLayoutCompileDesc& desc) override;
	std::unique_ptr<RenderPipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) override;
	std::unique_ptr<RenderPipeline> CreateComputePipeline(const ComputePipelineDesc& desc) override;

  private:
	D3D12Rhi* m_rhi = nullptr;
};
