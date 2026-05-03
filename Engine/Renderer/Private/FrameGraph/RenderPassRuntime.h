#pragma once

#include "RendererAPI.h"

#include "RHI/Public/Interop/RenderHardwareInterface.h"

class GBufferPass;
class DeferredLightingPass;
class ComputeClearPass;

template <typename TPass> struct RenderPassRuntimeTraits;

struct SPARKLE_RENDERER_API GBufferPassRuntime
{
	RenderBindingLayout& BindingLayout;
	RenderPipelineState& PipelineState;
};

struct SPARKLE_RENDERER_API DeferredLightingPassRuntime
{
	RenderBindingLayout& BindingLayout;
	RenderPipelineState& PipelineState;
};

struct SPARKLE_RENDERER_API ComputeClearPassRuntime
{
	RenderBindingLayout& BindingLayout;
	RenderPipelineState& PipelineState;
};

template <> struct RenderPassRuntimeTraits<GBufferPass>
{
	using RuntimeType = GBufferPassRuntime;
};

template <> struct RenderPassRuntimeTraits<DeferredLightingPass>
{
	using RuntimeType = DeferredLightingPassRuntime;
};

template <> struct RenderPassRuntimeTraits<ComputeClearPass>
{
	using RuntimeType = ComputeClearPassRuntime;
};
