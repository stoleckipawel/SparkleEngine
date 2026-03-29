#pragma once

#include "RendererAPI.h"

#include <tuple>

class D3D12BindingLayout;
class D3D12PipelineState;
class ComputeClearPass;
class ForwardOpaquePass;
class ShadowOpaquePass;

template <typename TPass> struct RenderPassRuntimeTraits;

struct SPARKLE_RENDERER_API ForwardOpaquePassRuntime
{
	D3D12BindingLayout& BindingLayout;
	D3D12PipelineState& PipelineState;
};

struct SPARKLE_RENDERER_API ShadowOpaquePassRuntime
{
	D3D12BindingLayout& BindingLayout;
	D3D12PipelineState& PipelineState;
};

struct SPARKLE_RENDERER_API ComputeClearPassRuntime
{
	D3D12BindingLayout& BindingLayout;
	D3D12PipelineState& PipelineState;
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