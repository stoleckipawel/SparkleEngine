#include "../PCH.h"
#include "PipelineStateManager.h"

#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Diagnostics/Verify.h"

namespace
{
	template <typename... TPasses>
	bool InitializeRuntimeStorage(
	    RenderHardwareInterface& rhi,
	    CookedShaderPackageCache& shaderPackages,
	    std::tuple<RenderPassRuntimeStorage<TPasses>...>& runtimeStorage,
	    std::string& outErrorMessage)
	{
		bool success = true;
		((success = success && RenderPassPipelineTraits<TPasses>::CreateRuntimeStorage(
		     rhi,
		     shaderPackages,
		     std::get<RenderPassRuntimeStorage<TPasses>>(runtimeStorage),
		     outErrorMessage)),
		 ...);
		return success;
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

CookedShaderReloadResult PipelineStateManager::ReloadCookedShaders() noexcept
{
	static const std::shared_ptr<spdlog::logger>& logger = Logging::GetOrCreateLogger("Renderer");
	SPDLOG_LOGGER_INFO(logger, "Reloading cooked shader packages and pipeline state runtimes");

	CookedShaderPackageCache nextShaderPackages;
	PassRuntimeStorageTuple nextRuntimeStorage{};
	std::string errorMessage;
	if (!TryInitializePassRuntimes(nextShaderPackages, nextRuntimeStorage, errorMessage))
	{
		SPDLOG_LOGGER_ERROR(logger, "Cooked shader reload rejected; keeping previous shader packages active. {}", errorMessage);
		return CookedShaderReloadResult::Failure(std::move(errorMessage));
	}

	m_runtimeRegistry.reset();
	m_shaderPackages.ReplaceWith(std::move(nextShaderPackages));
	m_runtimeStorage = std::move(nextRuntimeStorage);
	m_runtimeRegistry.emplace(BuildRuntimeRegistry(m_runtimeStorage));

	SPDLOG_LOGGER_INFO(logger, "Cooked shader reload complete (generation={})", m_shaderPackages.GetGeneration());
	return CookedShaderReloadResult::Success();
}

bool PipelineStateManager::TryInitializePassRuntimes(
    CookedShaderPackageCache& shaderPackages,
    PassRuntimeStorageTuple& runtimeStorage,
    std::string& outErrorMessage)
{
	if (!InitializeRuntimeStorage(*m_rhi, shaderPackages, runtimeStorage, outErrorMessage))
	{
		return false;
	}

	outErrorMessage.clear();
	return true;
}

void PipelineStateManager::InitializePassRuntimes()
{
	std::string errorMessage;
	if (!TryInitializePassRuntimes(m_shaderPackages, m_runtimeStorage, errorMessage))
	{
		Diagnostics::Fail(Logging::GetOrCreateLogger("Renderer"), __FILE__, __LINE__, errorMessage);
	}
	m_runtimeRegistry.emplace(BuildRuntimeRegistry(m_runtimeStorage));
}
