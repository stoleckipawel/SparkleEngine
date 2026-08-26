#pragma once

#include "RHI/Public/Pipeline/RhiPipelineDesc.h"

#include <cstdint>

class RenderBindingLayout;
class RenderPipeline;
class RayTracingPipeline;

struct RasterPassRuntime
{
	RenderBindingLayout& BindingLayout;
	RenderPipeline& Pipeline;
};

struct ComputePassPipelineRuntime
{
	RenderBindingLayout& BindingLayout;
	RenderPipeline& Pipeline;
};

struct RayTracingPassPipelineRuntime
{
	RenderBindingLayout& BindingLayout;
	RayTracingPipeline& Pipeline;
	std::uint64_t Generation = 0;
};
