#include "PCH.h"

#include "Device/RenderDeviceServicesState.h"

#include "Device/RenderDeviceBackendServices.h"

#include <utility>

RenderDeviceServicesState::RenderDeviceServicesState() noexcept = default;

RenderDeviceServicesState::~RenderDeviceServicesState() noexcept
{
	m_owner.AssertAccess();
}

void RenderDeviceServicesState::SetBackendServices(
    std::unique_ptr<RenderDeviceBackendServices> backendServices) noexcept
{
	m_owner.AssertAccess();
	m_backendServices = std::move(backendServices);
}

RenderDeviceBackendServices& RenderDeviceServicesState::GetBackendServices(
    std::source_location location) noexcept
{
	m_owner.AssertAccess(location);
	return *m_backendServices;
}

const RenderDeviceBackendServices& RenderDeviceServicesState::GetBackendServices(
    std::source_location location) const noexcept
{
	m_owner.AssertAccess(location);
	return *m_backendServices;
}
