#pragma once

#include "Pipeline/RhiPipelineService.h"

#include <memory>

struct ComputePipelineStateDesc;
struct GraphicsPipelineStateDesc;
struct RenderBindingLayoutCompileDesc;
class RenderBindingLayout;
class RenderPipelineState;
class VulkanRhi;

class VulkanPipelineService final : public RhiPipelineService
{
  public:
	explicit VulkanPipelineService(VulkanRhi& rhi) noexcept;

	std::unique_ptr<RenderBindingLayout> CreateBindingLayout(const RenderBindingLayoutCompileDesc& desc) override;
	std::unique_ptr<RenderPipelineState> CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc) override;
	std::unique_ptr<RenderPipelineState> CreateComputePipelineState(const ComputePipelineStateDesc& desc) override;

  private:
	VulkanRhi* m_rhi = nullptr;
};
