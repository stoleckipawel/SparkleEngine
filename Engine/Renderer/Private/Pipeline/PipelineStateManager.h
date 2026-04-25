#pragma once

#include "Pipeline/RenderPassPipelineTraits.h"
#include "FrameGraph/RenderPassRuntime.h"

#include <cassert>
#include <memory>
#include <optional>
#include <tuple>

class RenderHardwareInterface;

class PipelineStateManager final
{
  public:
	explicit PipelineStateManager(RenderHardwareInterface& rhi) noexcept;
	~PipelineStateManager() noexcept;

	PipelineStateManager(const PipelineStateManager&) = delete;
	PipelineStateManager& operator=(const PipelineStateManager&) = delete;
	PipelineStateManager(PipelineStateManager&&) = delete;
	PipelineStateManager& operator=(PipelineStateManager&&) = delete;

	const RenderPassRuntimeRegistry& GetRuntimeRegistry() const noexcept;
	std::uint64_t GetShaderPackageGeneration() const noexcept { return m_shaderPackages.GetGeneration(); }
	void ReloadCookedShaders() noexcept;

	template <typename TPass> const typename RenderPassRuntimeTraits<TPass>::RuntimeType& GetPassRuntime() const noexcept
	{
		assert(m_runtimeRegistry.has_value());
		return m_runtimeRegistry->GetPassRuntime<TPass>();
	}

  private:
	using PassRuntimeStorageTuple = std::tuple<
	    RenderPassRuntimeStorage<ForwardOpaquePass>,
	    RenderPassRuntimeStorage<ShadowOpaquePass>,
	    RenderPassRuntimeStorage<ComputeClearPass>>;

	template <typename TPass> RenderPassRuntimeStorage<TPass>& GetRuntimeStorage() noexcept
	{
		return std::get<RenderPassRuntimeStorage<TPass>>(m_runtimeStorage);
	}

	template <typename TPass> const RenderPassRuntimeStorage<TPass>& GetRuntimeStorage() const noexcept
	{
		return std::get<RenderPassRuntimeStorage<TPass>>(m_runtimeStorage);
	}

	void InitializePassRuntimes();

	RenderHardwareInterface* m_rhi = nullptr;
	CookedShaderPackageCache m_shaderPackages;
	PassRuntimeStorageTuple m_runtimeStorage;
	std::optional<RenderPassRuntimeRegistry> m_runtimeRegistry;
};