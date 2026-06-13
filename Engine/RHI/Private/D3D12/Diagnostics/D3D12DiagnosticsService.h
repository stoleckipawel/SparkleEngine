#pragma once

#include "Diagnostics/RhiDiagnosticsService.h"

class D3D12RenderHardwareInterface;

class D3D12DiagnosticsService final : public RhiDiagnosticsService
{
  public:
	explicit D3D12DiagnosticsService(D3D12RenderHardwareInterface& owner) noexcept;

	RenderDiagnostics& GetDiagnostics() noexcept override;
	const RenderDiagnostics& GetDiagnostics() const noexcept override;

  private:
	D3D12RenderHardwareInterface* m_owner = nullptr;
};
