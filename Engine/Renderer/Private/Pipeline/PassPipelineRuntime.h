#pragma once

#include "RHI/Public/Pipeline/RhiPipelineDesc.h"

#include <cstdint>

class RenderBindingLayout;
class RenderPipeline;
class RayTracingPipeline;

struct RasterPassRuntime
{
	const RenderBindingLayout& BindingLayout;
	const RenderPipeline& Pipeline;
};

struct ComputePassPipelineRuntime
{
	const RenderBindingLayout& BindingLayout;
	const RenderPipeline& Pipeline;
};

struct RayTracingPassPipelineRuntime
{
	const RenderBindingLayout& BindingLayout;
	const RayTracingPipeline& Pipeline;
	std::uint64_t Generation = 0;
};
