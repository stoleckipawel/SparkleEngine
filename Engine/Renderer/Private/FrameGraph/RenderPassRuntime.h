#pragma once

#include "RHI/Public/Device/RenderHardwareInterface.h"

class GBufferPass;
class DirectLightingPass;
class IndirectLightingPass;
class LightingCompositePass;
class SkyPass;
class VisualizeBuffersPass;
class ComputeClearPass;

template <typename TPass> struct RenderPassRuntimeTraits;

struct RasterPassRuntime
{
	RenderBindingLayout& BindingLayout;
	RenderPipelineState& PipelineState;
	RenderPipelineState* WireframePipelineState = nullptr;
};

using GBufferPassRuntime = RasterPassRuntime;

struct DirectLightingPassRuntime
{
	RenderBindingLayout& BindingLayout;
	RenderPipelineState& PipelineState;
};

struct IndirectLightingPassRuntime
{
	RenderBindingLayout& BindingLayout;
	RenderPipelineState& PipelineState;
};

struct LightingCompositePassRuntime
{
	RenderBindingLayout& BindingLayout;
	RenderPipelineState& PipelineState;
};

struct SkyPassRuntime
{
	RenderBindingLayout& BindingLayout;
	RenderPipelineState& PipelineState;
};

struct VisualizeBuffersPassRuntime
{
	RenderBindingLayout& BindingLayout;
	RenderPipelineState& PipelineState;
};

struct ComputeClearPassRuntime
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
