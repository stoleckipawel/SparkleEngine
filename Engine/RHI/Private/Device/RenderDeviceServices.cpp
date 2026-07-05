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

static void ValidateBackBufferFormat(PixelFormat backBufferFormat) noexcept
{
	if (!RhiPresentationDefaults::IsSupportedBackBufferFormat(backBufferFormat))
	{
		const std::string message =
		    std::string("Unsupported back buffer format for present swapchain: ") + PixelFormatName(backBufferFormat);
		Diagnostics::Fail(g_rhiServicesLogger, __FILE__, __LINE__, message);
	}
}

RenderDeviceServices::RenderDeviceServices() noexcept = default;

RenderDeviceServices::~RenderDeviceServices() noexcept = default;

std::unique_ptr<RenderDeviceServices> RenderDeviceServices::Create(Window& window) noexcept
{
	return Create(window, ResolveDefaultRhiBackendApi(), RhiPresentationDefaults::BackBufferFormat);
}

std::unique_ptr<RenderDeviceServices> RenderDeviceServices::Create(Window& window, ERhiBackendApi backendApi) noexcept
{
	return Create(window, backendApi, RhiPresentationDefaults::BackBufferFormat);
}

std::unique_ptr<RenderDeviceServices> RenderDeviceServices::Create(
    Window& window,
    PixelFormat backBufferFormat) noexcept
{
	return Create(window, ResolveDefaultRhiBackendApi(), backBufferFormat);
}

std::unique_ptr<RenderDeviceServices> RenderDeviceServices::Create(
    Window& window,
    ERhiBackendApi backendApi,
    PixelFormat backBufferFormat) noexcept
{
	ValidateBackBufferFormat(backBufferFormat);

	auto services = std::unique_ptr<RenderDeviceServices>(new RenderDeviceServices());
	switch (backendApi)
	{
		case ERhiBackendApi::D3D12:
		#if SPARKLE_RHI_WITH_D3D12
			services->m_backend = CreateD3D12RenderDeviceServices(window, backBufferFormat);
			break;
		#else
			FailUnsupportedRhiBackend(backendApi);
			break;
		#endif
		case ERhiBackendApi::Vulkan:
		#if SPARKLE_RHI_WITH_VULKAN
			services->m_backend = CreateVulkanRenderDeviceServices(window, backBufferFormat);
			break;
		#else
			FailUnsupportedRhiBackend(backendApi);
			break;
		#endif
		case ERhiBackendApi::Unknown:
		default:
			FailUnsupportedRhiBackend(backendApi);
	}

	return services;
}

const RhiCapabilities& RenderDeviceServices::GetCapabilities() const noexcept
{
	const RenderDeviceBackendServices& backend = *m_backend;
	return backend.GetRenderHardwareInterface().GetCapabilities();
}

RenderHardwareInterface& RenderDeviceServices::GetRenderHardwareInterface() noexcept
{
	return m_backend->GetRenderHardwareInterface();
}

const RenderHardwareInterface& RenderDeviceServices::GetRenderHardwareInterface() const noexcept
{
	const RenderDeviceBackendServices& backend = *m_backend;
	return backend.GetRenderHardwareInterface();
}

RhiImGuiRenderer& RenderDeviceServices::GetImGuiRenderer() noexcept
{
	return m_backend->GetImGuiRenderer();
}

void RenderDeviceServices::WaitForIdle() noexcept
{
	m_backend->Flush();
}

void RenderDeviceServices::Flush() noexcept
{
	m_backend->Flush();
}

void RenderDeviceServices::ResizeSwapChain() noexcept
{
	m_backend->ResizeSwapChain();
}

void RenderDeviceServices::BeginFrame() noexcept
{
	m_backend->BeginFrame();
}

RenderCommandList& RenderDeviceServices::GetCurrentGraphicsCommandList() noexcept
{
	return m_backend->GetCurrentGraphicsCommandList();
}

RenderCommandList& RenderDeviceServices::GetGraphicsCommandList(std::uint32_t frameIndex) noexcept
{
	return m_backend->GetGraphicsCommandList(frameIndex);
}

void RenderDeviceServices::SubmitFrame() noexcept
{
	m_backend->SubmitFrame();
}

void RenderDeviceServices::AdvanceFrameInFlight() noexcept
{
	m_backend->AdvanceFrameInFlight();
}

void RenderDeviceServices::CloseExecuteAndFlushCurrentFrame() noexcept
{
	m_backend->CloseExecuteAndFlushCurrentFrame();
}
