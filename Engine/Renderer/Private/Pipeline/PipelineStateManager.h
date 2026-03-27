#pragma once

#include "Pipeline/RenderPassPipelineTraits.h"
#include "FrameGraph/RenderPassRuntime.h"

#include <cassert>
#include <memory>
#include <optional>
#include <tuple>

class D3D12Rhi;

class PipelineStateManager final
{
  public:
	explicit PipelineStateManager(D3D12Rhi& rhi) noexcept;
	~PipelineStateManager() noexcept;

	PipelineStateManager(const PipelineStateManager&) = delete;
	PipelineStateManager& operator=(const PipelineStateManager&) = delete;
	PipelineStateManager(PipelineStateManager&&) = delete;
	PipelineStateManager& operator=(PipelineStateManager&&) = delete;

	const RenderPassRuntimeRegistry& GetRuntimeRegistry() const noexcept;

	template <typename TPass>
	const typename RenderPassRuntimeTraits<TPass>::RuntimeType& GetPassRuntime() const noexcept
	{
		assert(m_runtimeRegistry.has_value());
		return m_runtimeRegistry->GetPassRuntime<TPass>();
	}

  private:
	using PassRuntimeStorageTuple = std::tuple<
	    RenderPassRuntimeStorage<ForwardOpaquePass>,
	    RenderPassRuntimeStorage<ShadowOpaquePass>,
	    RenderPassRuntimeStorage<ComputeClearPass>>;

	template <typename TPass>
	RenderPassRuntimeStorage<TPass>& GetRuntimeStorage() noexcept
	{
		return std::get<RenderPassRuntimeStorage<TPass>>(m_runtimeStorage);
	}

	template <typename TPass>
	const RenderPassRuntimeStorage<TPass>& GetRuntimeStorage() const noexcept
	{
		return std::get<RenderPassRuntimeStorage<TPass>>(m_runtimeStorage);
	}

	void InitializePassRuntimes();

	D3D12Rhi* m_rhi = nullptr;
	PassRuntimeStorageTuple m_runtimeStorage;
	std::optional<RenderPassRuntimeRegistry> m_runtimeRegistry;
};