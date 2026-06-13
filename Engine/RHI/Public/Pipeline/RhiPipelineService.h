#pragma once

#include "../Pipeline/RhiPipelineStateDesc.h"
#include "../RHIAPI.h"

#include <memory>

class RenderBindingLayout;
class RenderPipelineState;

class SPARKLE_RHI_API RhiPipelineService
{
  public:
	virtual ~RhiPipelineService() noexcept = default;

	virtual std::unique_ptr<RenderBindingLayout> CreateBindingLayout(const RenderBindingLayoutCompileDesc& desc) = 0;
	virtual std::unique_ptr<RenderPipelineState> CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc) = 0;
	virtual std::unique_ptr<RenderPipelineState> CreateComputePipelineState(const ComputePipelineStateDesc& desc) = 0;
};
