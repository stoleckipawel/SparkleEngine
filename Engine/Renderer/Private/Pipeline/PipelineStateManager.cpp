#include "../PCH.h"
#include "PipelineStateManager.h"

#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"

PipelineStateManager::PipelineStateManager(RenderHardwareInterface& renderHardwareInterface) noexcept :
	m_renderHardwareInterface(&renderHardwareInterface)
{
}

PipelineStateManager::~PipelineStateManager() noexcept = default;

CookedShaderReloadResult PipelineStateManager::ReloadCookedShaders() noexcept
{
	static const std::shared_ptr<spdlog::logger>& logger = Logging::GetOrCreateLogger("Renderer");
	SPDLOG_LOGGER_INFO(logger, "Reloading cooked shader packages and clearing lazy pipeline state runtimes");

	CookedShaderPackageCache nextShaderPackages;
	m_shaderPackages.ReplaceWith(std::move(nextShaderPackages));
	m_runtimeStorageByPassName.clear();

	SPDLOG_LOGGER_INFO(logger, "Cooked shader reload complete (generation={})", m_shaderPackages.GetGeneration());
	return CookedShaderReloadResult::Success();
}

[[noreturn]] void PipelineStateManager::HandleRuntimeCreationFailure(const std::string& errorMessage) const
{
	Diagnostics::Fail(Logging::GetOrCreateLogger("Renderer"), __FILE__, __LINE__, errorMessage);
}
