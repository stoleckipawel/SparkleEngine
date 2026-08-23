#pragma once

#include "RHI/Public/Pipeline/RhiPipelineDesc.h"

class RenderBindingLayout;
class RenderPipeline;

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
