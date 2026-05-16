#include "PCH.h"

#include "Device/RenderDeviceServices.h"
#include "Device/RenderDeviceBackendFactory.h"
#include "Device/RenderDeviceBackendServices.h"

#include "Time/Timer.h"
#include "Window/Window.h"

#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Diagnostics/Trace.h"
#include "Core/Public/Diagnostics/Verify.h"
#include <string>

static std::shared_ptr<spdlog::logger> g_rhiServicesLogger = Logging::GetOrCreateLogger("RHI.Services");

static void FailUnsupportedRhiBackend(ERhiBackendApi api) noexcept
{
	const std::string message = std::string("RHI backend '") + RhiBackendApiToString(api) + "' is not implemented by RenderDeviceServices yet.";
	Diagnostics::Fail(g_rhiServicesLogger, __FILE__, __LINE__, message);
}

struct RenderDeviceServices::Impl
{
	std::unique_ptr<RenderDeviceBackendServices> backend;
};

RenderDeviceServices::RenderDeviceServices() noexcept = default;

RenderDeviceServices::~RenderDeviceServices() noexcept = default;

std::unique_ptr<RenderDeviceServices> RenderDeviceServices::Create(Timer& timer, Window& window) noexcept
{
	return Create(timer, window, ResolveDefaultRhiBackendSelection());
}

std::unique_ptr<RenderDeviceServices> RenderDeviceServices::Create(Timer& timer, Window& window, RhiBackendSelection selection) noexcept
{
	SPARKLE_CPU_SCOPE("RHI.CreateBackend");
	SPDLOG_LOGGER_INFO(g_rhiServicesLogger, "Creating RHI backend: {}", RhiBackendApiToString(selection.Api));

	auto services = std::unique_ptr<RenderDeviceServices>(new RenderDeviceServices());
	services->m_impl = std::make_unique<Impl>();
	switch (selection.Api)
	{
		case ERhiBackendApi::D3D12:
		#if SPARKLE_RHI_WITH_D3D12
			services->m_impl->backend = CreateD3D12RenderDeviceServices(timer, window);
			break;
		#else
			FailUnsupportedRhiBackend(selection.Api);
			break;
		#endif
		case ERhiBackendApi::Vulkan:
		case ERhiBackendApi::Unknown:
		default:
			FailUnsupportedRhiBackend(selection.Api);
	}
	return services;
}

RenderHardwareInterface& RenderDeviceServices::GetRenderHardwareInterface() noexcept
{
	return m_impl->backend->GetRenderHardwareInterface();
}

const RenderHardwareInterface& RenderDeviceServices::GetRenderHardwareInterface() const noexcept
{
	const RenderDeviceBackendServices& backend = *m_impl->backend;
	return backend.GetRenderHardwareInterface();
}

RenderDiagnostics& RenderDeviceServices::GetDiagnostics() noexcept
{
	return m_impl->backend->GetDiagnostics();
}

const RenderDiagnostics& RenderDeviceServices::GetDiagnostics() const noexcept
{
	const RenderDeviceBackendServices& backend = *m_impl->backend;
	return backend.GetDiagnostics();
}

void RenderDeviceServices::Flush() noexcept
{
	m_impl->backend->Flush();
}

void RenderDeviceServices::ResizeSwapChain() noexcept
{
	m_impl->backend->ResizeSwapChain();
}

void RenderDeviceServices::BeginFrame() noexcept
{
	m_impl->backend->BeginFrame();
}

RenderCommandList& RenderDeviceServices::GetCurrentGraphicsCommandList() noexcept
{
	return m_impl->backend->GetCurrentGraphicsCommandList();
}

void RenderDeviceServices::SubmitFrame() noexcept
{
	m_impl->backend->SubmitFrame();
}

void RenderDeviceServices::AdvanceFrameInFlight() noexcept
{
	m_impl->backend->AdvanceFrameInFlight();
}

void RenderDeviceServices::UpdatePerFrameConstants(std::uint32_t renderViewMode) noexcept
{
	m_impl->backend->UpdatePerFrameConstants(renderViewMode);
}

void RenderDeviceServices::CloseExecuteAndFlushCurrentFrame() noexcept
{
	m_impl->backend->CloseExecuteAndFlushCurrentFrame();
}
