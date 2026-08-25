#pragma once

#include "RHI/Public/Pipeline/RhiPipelineDesc.h"

#include <cstdint>

class RenderBindingLayout;
class RenderPipeline;
class RayTracingPipeline;
class RayTracingShaderTable;

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
	RayTracingShaderTable& ShaderTable;
	std::uint64_t Generation = 0;
};
