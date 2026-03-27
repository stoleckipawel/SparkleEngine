#pragma once

#include "RendererAPI.h"
#include "FrameGraph/RenderPassRuntime.h"

class D3D12ConstantBufferManager;
class D3D12DescriptorHeapManager;
class D3D12SamplerLibrary;

// Stable execute-time runtime services exposed to authored render passes.
//
// This surface owns the renderer services that remain intentionally global at
// pass execution time, plus typed runtime lookup by pass type.
//
// Keep this struct focused. New fields should only be added when they are
// genuinely stable execute-time services shared across authored passes.
struct SPARKLE_RENDERER_API RenderPassContext
{
	D3D12DescriptorHeapManager& DescriptorHeapManager;
	D3D12ConstantBufferManager& ConstantBufferManager;
	D3D12SamplerLibrary& SamplerLibrary;
	const RenderPassRuntimeRegistry& RuntimeRegistry;

	template <typename TPass>
	const typename RenderPassRuntimeTraits<TPass>::RuntimeType& GetPassRuntime() const noexcept
	{
		return RuntimeRegistry.GetPassRuntime<TPass>();
	}
};
