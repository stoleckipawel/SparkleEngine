#pragma once

#include "Pipeline/RhiPipelineService.h"

#include <memory>

struct ComputePipelineDesc;
struct GraphicsPipelineDesc;
struct RenderBindingLayoutCompileDesc;
class RenderBindingLayout;
class RenderPipeline;
class VulkanRhi;

class VulkanPipelineService final : public RhiPipelineService
{
  public:
	explicit VulkanPipelineService(VulkanRhi& rhi) noexcept;

	std::unique_ptr<RenderBindingLayout> CreateBindingLayout(const RenderBindingLayoutCompileDesc& desc) override;
	std::unique_ptr<RenderPipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) override;
	std::unique_ptr<RenderPipeline> CreateComputePipeline(const ComputePipelineDesc& desc) override;

  private:
	VulkanRhi* m_rhi = nullptr;
};
