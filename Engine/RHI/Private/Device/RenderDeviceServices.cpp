#include "PCH.h"

#include "Device/RenderDeviceServices.h"
#include "Device/RenderDeviceBackendFactory.h"
#include "Device/RenderDeviceBackendServices.h"
#include "Presentation/RhiPresentationDefaults.h"
#include "Shaders/CookedShaderPackageUtils.h"

#include "Window/Window.h"

#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Diagnostics/Verify.h"
#include <string>

static std::shared_ptr<spdlog::logger> g_rhiServicesLogger = Logging::GetOrCreateLogger("RHI.Services");

static void FailUnsupportedRhiBackend(ERhiBackendApi api) noexcept
{
	const std::string message =
	    std::string("RHI backend '") + RhiBackendApiToString(api) + "' is not available in this RenderDeviceServices build.";
	Diagnostics::Fail(g_rhiServicesLogger, __FILE__, __LINE__, message);
}

static void ValidateRenderDeviceSettings(const RenderDeviceSettings& settings) noexcept
{
	if (!RhiPresentationDefaults::IsSupportedBackBufferFormat(settings.BackBufferFormat))
	{
		const std::string message =
		    std::string("Unsupported back buffer format for present swapchain: ") + PixelFormatName(settings.BackBufferFormat);
		Diagnostics::Fail(g_rhiServicesLogger, __FILE__, __LINE__, message);
	}
}

struct RenderDeviceServices::Impl
{
	std::unique_ptr<RenderDeviceBackendServices> backend;
};

RenderDeviceServices::RenderDeviceServices() noexcept = default;

RenderDeviceServices::~RenderDeviceServices() noexcept = default;

std::unique_ptr<RenderDeviceServices> RenderDeviceServices::Create(Window& window) noexcept
{
	return Create(window, ResolveDefaultRhiBackendSelection(), RenderDeviceSettings{});
}

std::unique_ptr<RenderDeviceServices> RenderDeviceServices::Create(Window& window, RhiBackendSelection selection) noexcept
{
	return Create(window, selection, RenderDeviceSettings{});
}

std::unique_ptr<RenderDeviceServices> RenderDeviceServices::Create(
    Window& window,
    const RenderDeviceSettings& settings) noexcept
{
	return Create(window, ResolveDefaultRhiBackendSelection(), settings);
}

std::unique_ptr<RenderDeviceServices> RenderDeviceServices::Create(
    Window& window,
    RhiBackendSelection selection,
    const RenderDeviceSettings& settings) noexcept
{
	ValidateRenderDeviceSettings(settings);

	auto services = std::unique_ptr<RenderDeviceServices>(new RenderDeviceServices());
	services->m_impl = std::make_unique<Impl>();
	switch (selection.Api)
	{
		case ERhiBackendApi::D3D12:
		#if SPARKLE_RHI_WITH_D3D12
			services->m_impl->backend = CreateD3D12RenderDeviceServices(window, settings);
			break;
		#else
			FailUnsupportedRhiBackend(selection.Api);
			break;
		#endif
		case ERhiBackendApi::Vulkan:
		#if SPARKLE_RHI_WITH_VULKAN
			services->m_impl->backend = CreateVulkanRenderDeviceServices(window, settings);
			break;
		#else
			FailUnsupportedRhiBackend(selection.Api);
			break;
		#endif
		case ERhiBackendApi::Unknown:
		default:
			FailUnsupportedRhiBackend(selection.Api);
	}

	return services;
}

const RhiCapabilities& RenderDeviceServices::GetCapabilities() const noexcept
{
	const RenderDeviceBackendServices& backend = *m_impl->backend;
	return backend.GetRenderHardwareInterface().GetCapabilities();
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

RhiImGuiRenderer& RenderDeviceServices::GetImGuiRenderer() noexcept
{
	return m_impl->backend->GetImGuiRenderer();
}

void RenderDeviceServices::WaitForIdle() noexcept
{
	m_impl->backend->Flush();
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

RenderCommandList& RenderDeviceServices::GetGraphicsCommandList(std::uint32_t frameIndex) noexcept
{
	return m_impl->backend->GetGraphicsCommandList(frameIndex);
}

void RenderDeviceServices::SubmitFrame() noexcept
{
	m_impl->backend->SubmitFrame();
}

void RenderDeviceServices::AdvanceFrameInFlight() noexcept
{
	m_impl->backend->AdvanceFrameInFlight();
}

void RenderDeviceServices::CloseExecuteAndFlushCurrentFrame() noexcept
{
	m_impl->backend->CloseExecuteAndFlushCurrentFrame();
}
