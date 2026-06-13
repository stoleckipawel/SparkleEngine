#include "Vulkan/Diagnostics/VulkanDiagnosticsService.h"

#include "Vulkan/VulkanRenderHardwareInterface.h"

VulkanDiagnosticsService::VulkanDiagnosticsService(VulkanRenderHardwareInterface& owner) noexcept : m_owner(&owner) {}

RenderDiagnostics& VulkanDiagnosticsService::GetDiagnostics() noexcept
{
	return m_owner->GetDiagnostics();
}

const RenderDiagnostics& VulkanDiagnosticsService::GetDiagnostics() const noexcept
{
	return m_owner->GetDiagnostics();
}
