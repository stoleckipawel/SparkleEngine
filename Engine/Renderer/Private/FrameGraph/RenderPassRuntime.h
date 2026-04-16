#pragma once

#include "RendererAPI.h"

#include "RHI/Public/Interop/RenderHardwareInterface.h"

#include <tuple>

class ComputeClearPass;
class ForwardOpaquePass;
class ShadowOpaquePass;

template <typename TPass> struct RenderPassRuntimeTraits;

struct SPARKLE_RENDERER_API ForwardOpaquePassRuntime
{
	RenderBindingLayout& BindingLayout;
	RenderPipelineState& PipelineState;
};

struct SPARKLE_RENDERER_API ShadowOpaquePassRuntime
{
	RenderBindingLayout& BindingLayout;
	RenderPipelineState& PipelineState;
};

struct SPARKLE_RENDERER_API ComputeClearPassRuntime
{
	RenderBindingLayout& BindingLayout;
	RenderPipelineState& PipelineState;
};

template <> struct RenderPassRuntimeTraits<ForwardOpaquePass>
{
	using RuntimeType = ForwardOpaquePassRuntime;
};

template <> struct RenderPassRuntimeTraits<ShadowOpaquePass>
{
	using RuntimeType = ShadowOpaquePassRuntime;
};

template <> struct RenderPassRuntimeTraits<ComputeClearPass>
{
	using RuntimeType = ComputeClearPassRuntime;
};

template <typename... TPasses> class TypedRenderPassRuntimeRegistry
{
  public:
	explicit TypedRenderPassRuntimeRegistry(typename RenderPassRuntimeTraits<TPasses>::RuntimeType... runtimes) noexcept :
	    m_runtimes(runtimes...)
	{
	}

	template <typename TPass> const typename RenderPassRuntimeTraits<TPass>::RuntimeType& GetPassRuntime() const noexcept
	{
		return std::get<typename RenderPassRuntimeTraits<TPass>::RuntimeType>(m_runtimes);
	}

  private:
	std::tuple<typename RenderPassRuntimeTraits<TPasses>::RuntimeType...> m_runtimes;
};

using RenderPassRuntimeRegistry = TypedRenderPassRuntimeRegistry<ForwardOpaquePass, ShadowOpaquePass, ComputeClearPass>;