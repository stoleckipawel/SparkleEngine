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

RendererPipelineDiagnosticsSnapshot PipelineStateManager::CaptureDiagnosticsSnapshot() const
{
	return RendererPipelineDiagnosticsSnapshot{
	    .Status = ERendererDiagnosticStatus::Available,
	    .ShaderPackageGeneration = m_shaderPackages.GetGeneration(),
	    .LazyRuntimeCount = static_cast<std::uint32_t>(m_runtimeStorageByPassName.size()),
	    .LastShaderPackageLoad = m_shaderPackages.GetLastLoadReport(),
	    .PipelineCacheStatus = ERendererDiagnosticStatus::Planned,
	    .PipelineCacheReason = "Pipeline cache stats are planned; shader package cache timing is available now."};
}

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
