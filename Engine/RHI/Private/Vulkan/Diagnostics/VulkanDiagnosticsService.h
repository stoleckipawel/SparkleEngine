#pragma once

#include "Diagnostics/RhiDiagnosticsService.h"

class VulkanRenderHardwareInterface;

class VulkanDiagnosticsService final : public RhiDiagnosticsService
{
  public:
	explicit VulkanDiagnosticsService(VulkanRenderHardwareInterface& owner) noexcept;

	RenderDiagnostics& GetDiagnostics() noexcept override;
	const RenderDiagnostics& GetDiagnostics() const noexcept override;

  private:
	VulkanRenderHardwareInterface* m_owner = nullptr;
};
