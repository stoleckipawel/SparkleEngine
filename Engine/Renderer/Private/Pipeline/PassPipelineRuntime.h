#pragma once

class RenderBindingLayout;
class RenderPipelineState;

struct RasterPassPipelineRuntime
{
	RenderBindingLayout& BindingLayout;
	RenderPipelineState& PipelineState;
	RenderPipelineState* WireframePipelineState = nullptr;
	RenderPipelineState* TwoSidedPipelineState = nullptr;
};

struct ComputePassPipelineRuntime
{
	RenderBindingLayout& BindingLayout;
	RenderPipelineState& PipelineState;
};
