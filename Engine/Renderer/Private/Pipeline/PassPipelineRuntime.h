#pragma once

class RenderBindingLayout;
class RenderPipelineState;

struct RasterPassPipelineRuntime
{
	RenderBindingLayout& BindingLayout;
	RenderPipelineState& PipelineState;
	RenderPipelineState* WireframePipelineState = nullptr;
};

struct ComputePassPipelineRuntime
{
	RenderBindingLayout& BindingLayout;
	RenderPipelineState& PipelineState;
};