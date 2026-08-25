#pragma once

#include "../Pipeline/RhiPipelineDesc.h"
#include "../RayTracing/RhiRayTracingPipelineDesc.h"
#include "../RHIAPI.h"

#include <memory>

class RenderBindingLayout;
class RenderPipeline;

class SPARKLE_RHI_API RhiPipelineService
{
public:
	virtual ~RhiPipelineService() noexcept = default;

	virtual std::unique_ptr<RenderBindingLayout> CreateBindingLayout(const RenderBindingLayoutCompileDesc& desc) = 0;
	virtual std::unique_ptr<RenderPipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) = 0;
	virtual std::unique_ptr<RenderPipeline> CreateComputePipeline(const ComputePipelineDesc& desc) = 0;
	virtual std::unique_ptr<RayTracingPipeline> CreateRayTracingPipeline(const RayTracingPipelineDesc& desc) = 0;
};
