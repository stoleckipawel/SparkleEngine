#pragma once

#include "Core/Public/Threading/ThreadOwnership.h"

#include <memory>
#include <source_location>

class RenderDeviceBackendServices;

class RenderDeviceServicesState final
{
public:
	RenderDeviceServicesState() noexcept;
	~RenderDeviceServicesState() noexcept;

	RenderDeviceServicesState(const RenderDeviceServicesState&) = delete;
	RenderDeviceServicesState& operator=(const RenderDeviceServicesState&) = delete;
	RenderDeviceServicesState(RenderDeviceServicesState&&) = delete;
	RenderDeviceServicesState& operator=(RenderDeviceServicesState&&) = delete;

	void SetBackendServices(std::unique_ptr<RenderDeviceBackendServices> backendServices) noexcept;
	RenderDeviceBackendServices& GetBackendServices(std::source_location location = std::source_location::current()) noexcept;
	const RenderDeviceBackendServices& GetBackendServices(std::source_location location = std::source_location::current()) const noexcept;

private:
	Threading::OwnerThread m_owner{"RenderDeviceServices"};
	std::unique_ptr<RenderDeviceBackendServices> m_backendServices;
};
