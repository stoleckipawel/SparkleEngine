#pragma once

class RenderBindingLayout;
class RenderPipeline;

struct RasterPassPipelineRuntime
{
	RenderBindingLayout& BindingLayout;
	RenderPipeline& Pipeline;
	RenderPipeline* WireframePipeline = nullptr;
	RenderPipeline* TwoSidedPipeline = nullptr;
};

struct ComputePassPipelineRuntime
{
	RenderBindingLayout& BindingLayout;
	RenderPipeline& Pipeline;
};
