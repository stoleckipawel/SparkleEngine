#include "../PCH.h"
#include "PipelineStateManager.h"

#include "Core/Public/Diagnostics/Logger.h"

namespace
{
	template <typename... TPasses>
	void InitializeRuntimeStorage(
	    RenderHardwareInterface& rhi,
	    CookedShaderPackageCache& shaderPackages,
	    std::tuple<RenderPassRuntimeStorage<TPasses>...>& runtimeStorage)
	{
		(RenderPassPipelineTraits<TPasses>::CreateRuntimeStorage(
		     rhi,
		     shaderPackages,
		     std::get<RenderPassRuntimeStorage<TPasses>>(runtimeStorage)),
		 ...);
	}

	template <typename... TPasses>
	RenderPassRuntimeRegistry BuildRuntimeRegistry(const std::tuple<RenderPassRuntimeStorage<TPasses>...>& runtimeStorage)
	{
		return RenderPassRuntimeRegistry(
		    RenderPassPipelineTraits<TPasses>::MakeRuntime(std::get<RenderPassRuntimeStorage<TPasses>>(runtimeStorage))...);
	}
}  // namespace

PipelineStateManager::PipelineStateManager(RenderHardwareInterface& rhi) noexcept : m_rhi(&rhi)
{
	InitializePassRuntimes();
}

PipelineStateManager::~PipelineStateManager() noexcept = default;

const RenderPassRuntimeRegistry& PipelineStateManager::GetRuntimeRegistry() const noexcept
{
	assert(m_runtimeRegistry.has_value());
	return *m_runtimeRegistry;
}

void PipelineStateManager::ReloadCookedShaders() noexcept
{
	static const std::shared_ptr<spdlog::logger>& logger = Logging::GetOrCreateLogger("Renderer");
	SPDLOG_LOGGER_INFO(logger, "Reloading cooked shader packages and pipeline state runtimes");

	m_runtimeRegistry.reset();
	m_runtimeStorage = PassRuntimeStorageTuple{};
	m_shaderPackages.Clear();
	InitializePassRuntimes();

	SPDLOG_LOGGER_INFO(logger, "Cooked shader reload complete (generation={})", m_shaderPackages.GetGeneration());
}

void PipelineStateManager::InitializePassRuntimes()
{
	InitializeRuntimeStorage(*m_rhi, m_shaderPackages, m_runtimeStorage);
	m_runtimeRegistry.emplace(BuildRuntimeRegistry(m_runtimeStorage));
}
