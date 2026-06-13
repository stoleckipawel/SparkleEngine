#pragma once

#include <memory>

struct ComputePipelineStateDesc;
struct GraphicsPipelineStateDesc;
struct RenderBindingLayoutCompileDesc;
class RenderBindingLayout;
class RenderPipelineState;
class VulkanRhi;

class VulkanPipelineService final
{
  public:
	explicit VulkanPipelineService(VulkanRhi& rhi) noexcept;

	std::unique_ptr<RenderBindingLayout> CreateBindingLayout(const RenderBindingLayoutCompileDesc& desc);
	std::unique_ptr<RenderPipelineState> CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc);
	std::unique_ptr<RenderPipelineState> CreateComputePipelineState(const ComputePipelineStateDesc& desc);

  private:
	VulkanRhi* m_rhi = nullptr;
};
