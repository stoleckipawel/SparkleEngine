#include "D3D12/Diagnostics/D3D12DiagnosticsService.h"

#include "D3D12/D3D12RenderHardwareInterface.h"

D3D12DiagnosticsService::D3D12DiagnosticsService(D3D12RenderHardwareInterface& owner) noexcept : m_owner(&owner) {}

RenderDiagnostics& D3D12DiagnosticsService::GetDiagnostics() noexcept
{
	return m_owner->GetDiagnostics();
}

const RenderDiagnostics& D3D12DiagnosticsService::GetDiagnostics() const noexcept
{
	return m_owner->GetDiagnostics();
}
