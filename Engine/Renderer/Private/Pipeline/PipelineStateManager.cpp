#include "../PCH.h"
#include "PipelineStateManager.h"

namespace
{
template <typename... TPasses>
void InitializeRuntimeStorage(D3D12Rhi& rhi, std::tuple<RenderPassRuntimeStorage<TPasses>...>& runtimeStorage)
{
	(RenderPassPipelineTraits<TPasses>::CreateRuntimeStorage(rhi, std::get<RenderPassRuntimeStorage<TPasses>>(runtimeStorage)), ...);
}

template <typename... TPasses>
RenderPassRuntimeRegistry BuildRuntimeRegistry(const std::tuple<RenderPassRuntimeStorage<TPasses>...>& runtimeStorage)
{
	return RenderPassRuntimeRegistry(
	    RenderPassPipelineTraits<TPasses>::MakeRuntime(std::get<RenderPassRuntimeStorage<TPasses>>(runtimeStorage))...);
}
}

PipelineStateManager::PipelineStateManager(D3D12Rhi& rhi) noexcept : m_rhi(&rhi)
{
	InitializePassRuntimes();
}

PipelineStateManager::~PipelineStateManager() noexcept = default;

const RenderPassRuntimeRegistry& PipelineStateManager::GetRuntimeRegistry() const noexcept
{
	assert(m_runtimeRegistry.has_value());
	return *m_runtimeRegistry;
}

void PipelineStateManager::InitializePassRuntimes()
{
	InitializeRuntimeStorage(*m_rhi, m_runtimeStorage);
	m_runtimeRegistry.emplace(BuildRuntimeRegistry(m_runtimeStorage));
}
