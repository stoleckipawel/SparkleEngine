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
	RhiPipelineService(const RhiPipelineService&) = delete;
	RhiPipelineService& operator=(const RhiPipelineService&) = delete;
	RhiPipelineService(RhiPipelineService&&) = delete;
	RhiPipelineService& operator=(RhiPipelineService&&) = delete;

	virtual std::unique_ptr<RenderBindingLayout> CreateBindingLayout(const RenderBindingLayoutCompileDesc& desc) = 0;
	virtual std::unique_ptr<RenderPipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) = 0;
	virtual std::unique_ptr<RenderPipeline> CreateComputePipeline(const ComputePipelineDesc& desc) = 0;
	virtual std::unique_ptr<RayTracingPipeline> CreateRayTracingPipeline(const RayTracingPipelineDesc& desc) = 0;

protected:
	RhiPipelineService() noexcept = default;
};
