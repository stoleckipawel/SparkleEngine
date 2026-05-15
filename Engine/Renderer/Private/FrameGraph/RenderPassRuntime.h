#pragma once

#include "RendererAPI.h"

#include "RHI/Public/Device/RenderHardwareInterface.h"

class GBufferPass;
class DirectLightingPass;
class IndirectLightingPass;
class LightingCompositePass;
class SkyPass;
class VisualizeBuffersPass;
class ComputeClearPass;

template <typename TPass> struct RenderPassRuntimeTraits;

struct SPARKLE_RENDERER_API RasterPassRuntime
{
	RenderBindingLayout& BindingLayout;
	RenderPipelineState& PipelineState;
	RenderPipelineState* WireframePipelineState = nullptr;
};

using GBufferPassRuntime = RasterPassRuntime;

struct SPARKLE_RENDERER_API DirectLightingPassRuntime
{
	RenderBindingLayout& BindingLayout;
	RenderPipelineState& PipelineState;
};

struct SPARKLE_RENDERER_API IndirectLightingPassRuntime
{
	RenderBindingLayout& BindingLayout;
	RenderPipelineState& PipelineState;
};

struct SPARKLE_RENDERER_API LightingCompositePassRuntime
{
	RenderBindingLayout& BindingLayout;
	RenderPipelineState& PipelineState;
};

struct SPARKLE_RENDERER_API SkyPassRuntime
{
	RenderBindingLayout& BindingLayout;
	RenderPipelineState& PipelineState;
};

struct SPARKLE_RENDERER_API VisualizeBuffersPassRuntime
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

template <> struct RenderPassRuntimeTraits<DirectLightingPass>
{
	using RuntimeType = DirectLightingPassRuntime;
};

template <> struct RenderPassRuntimeTraits<IndirectLightingPass>
{
	using RuntimeType = IndirectLightingPassRuntime;
};

template <> struct RenderPassRuntimeTraits<LightingCompositePass>
{
	using RuntimeType = LightingCompositePassRuntime;
};

template <> struct RenderPassRuntimeTraits<SkyPass>
{
	using RuntimeType = SkyPassRuntime;
};

template <> struct RenderPassRuntimeTraits<VisualizeBuffersPass>
{
	using RuntimeType = VisualizeBuffersPassRuntime;
};

template <> struct RenderPassRuntimeTraits<ComputeClearPass>
{
	using RuntimeType = ComputeClearPassRuntime;
};
