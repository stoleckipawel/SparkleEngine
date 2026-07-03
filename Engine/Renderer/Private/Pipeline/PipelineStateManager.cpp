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
	CookedShaderPackageCache nextShaderPackages;
	m_shaderPackages.ReplaceWith(std::move(nextShaderPackages));
	m_runtimeStorageByPassName.clear();

	return CookedShaderReloadResult::Success();
}

[[noreturn]] void PipelineStateManager::HandleRuntimeCreationFailure(const std::string& errorMessage) const
{
	Diagnostics::Fail(Logging::GetOrCreateLogger("Renderer"), __FILE__, __LINE__, errorMessage);
}
