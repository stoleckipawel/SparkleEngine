#include "PCH.h"

#include "Device/RenderDeviceServicesState.h"

#include "Device/RenderDeviceBackendServices.h"

#include <utility>

RenderDeviceServicesState::RenderDeviceServicesState() noexcept = default;

RenderDeviceServicesState::~RenderDeviceServicesState() noexcept
{
	m_owner.AssertAccess();
}

void RenderDeviceServicesState::SetBackend(
    std::unique_ptr<RenderDeviceBackendServices> backend) noexcept
{
	m_owner.AssertAccess();
	m_backend = std::move(backend);
}

RenderDeviceBackendServices& RenderDeviceServicesState::GetBackend(
    std::source_location location) noexcept
{
	m_owner.AssertAccess(location);
	return *m_backend;
}

const RenderDeviceBackendServices& RenderDeviceServicesState::GetBackend(
    std::source_location location) const noexcept
{
	m_owner.AssertAccess(location);
	return *m_backend;
}
