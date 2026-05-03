#pragma once

#include "RendererAPI.h"

#include "RHI/Public/Interop/RenderHardwareInterface.h"

#include <tuple>

class ComputeClearPass;
class DeferredLightingPass;
class GBufferPass;

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

using RenderPassRuntimeRegistry = TypedRenderPassRuntimeRegistry<GBufferPass, DeferredLightingPass, ComputeClearPass>;
