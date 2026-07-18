#include "PCH.h"

#include "Device/RenderDeviceServices.h"
#include "Device/RenderDeviceBackendFactory.h"
#include "Device/RenderDeviceBackendServices.h"
#include "Presentation/RhiPresentationDefaults.h"
#include "Shaders/CookedShaderPackageUtils.h"

#include "Window/Window.h"

#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "Core/Public/Threading/ThreadOwnership.h"
#include <string>
#include <utility>

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

class RenderDeviceServicesState final
{
  public:
	~RenderDeviceServicesState() noexcept { m_owner.AssertAccess(); }

	void SetBackend(std::unique_ptr<RenderDeviceBackendServices> backend) noexcept
	{
		m_owner.AssertAccess();
		m_backend = std::move(backend);
	}

	RenderDeviceBackendServices& Backend(std::source_location location = std::source_location::current()) noexcept
	{
		m_owner.AssertAccess(location);
		return *m_backend;
	}

	const RenderDeviceBackendServices& Backend(std::source_location location = std::source_location::current()) const noexcept
	{
		m_owner.AssertAccess(location);
		return *m_backend;
	}

  private:
	Threading::OwnerThread m_owner{"RenderDeviceServices"};
	std::unique_ptr<RenderDeviceBackendServices> m_backend;
};

RenderDeviceServices::RenderDeviceServices() noexcept : m_state(std::make_unique<RenderDeviceServicesState>()) {}

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
    PixelFormat backBufferFormat,
    RhiExternalFeatureHooks externalFeatureHooks) noexcept
{
	ValidateBackBufferFormat(backBufferFormat);

	auto services = std::unique_ptr<RenderDeviceServices>(new RenderDeviceServices());
	switch (backendApi)
	{
		case ERhiBackendApi::D3D12:
		#if SPARKLE_RHI_WITH_D3D12
			services->m_state->SetBackend(CreateD3D12RenderDeviceServices(window, backBufferFormat, externalFeatureHooks));
			break;
		#else
			FailUnsupportedRhiBackend(backendApi);
			break;
		#endif
		case ERhiBackendApi::Vulkan:
		#if SPARKLE_RHI_WITH_VULKAN
			services->m_state->SetBackend(CreateVulkanRenderDeviceServices(window, backBufferFormat, externalFeatureHooks));
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
	const RenderDeviceBackendServices& backend = m_state->Backend();
	return backend.GetRenderHardwareInterface().GetCapabilities();
}

RenderHardwareInterface& RenderDeviceServices::GetRenderHardwareInterface() noexcept
{
	return m_state->Backend().GetRenderHardwareInterface();
}

const RenderHardwareInterface& RenderDeviceServices::GetRenderHardwareInterface() const noexcept
{
	const RenderDeviceBackendServices& backend = m_state->Backend();
	return backend.GetRenderHardwareInterface();
}

RhiImGuiRenderer& RenderDeviceServices::GetImGuiRenderer() noexcept
{
	return m_state->Backend().GetImGuiRenderer();
}

void RenderDeviceServices::WaitForIdle() noexcept
{
	m_state->Backend().WaitForIdle();
}

void RenderDeviceServices::ResizeSwapChain() noexcept
{
	m_state->Backend().ResizeSwapChain();
}

void RenderDeviceServices::BeginFrame() noexcept
{
	RenderDeviceBackendServices& backend = m_state->Backend();
	backend.BeginFrame();
	RenderHardwareInterface& renderHardwareInterface = backend.GetRenderHardwareInterface();
	renderHardwareInterface.GetDescriptorService().BeginFrame(renderHardwareInterface.GetCurrentFrameIndex());
}

RenderCommandList& RenderDeviceServices::GetCurrentGraphicsCommandList() noexcept
{
	return m_state->Backend().GetCurrentGraphicsCommandList();
}

RenderCommandList& RenderDeviceServices::GetGraphicsCommandList(std::uint32_t frameIndex) noexcept
{
	return m_state->Backend().GetGraphicsCommandList(frameIndex);
}

RenderCommandList& RenderDeviceServices::BeginCommandList(ERhiQueueType queueType) noexcept
{
	return m_state->Backend().BeginCommandList(queueType);
}

RhiSubmissionToken RenderDeviceServices::SubmitCommandList(
	RenderCommandList& commandList,
	std::span<const RhiSubmissionToken> waitTokens) noexcept
{
	return m_state->Backend().SubmitCommandList(commandList, waitTokens);
}

void RenderDeviceServices::QueueWait(ERhiQueueType waitQueue, RhiSubmissionToken executionToken) noexcept
{
	m_state->Backend().QueueWait(waitQueue, executionToken);
}

void RenderDeviceServices::WaitForSubmission(RhiSubmissionToken token) noexcept
{
	m_state->Backend().WaitForSubmission(token);
}

bool RenderDeviceServices::IsSubmissionComplete(RhiSubmissionToken token) const noexcept
{
	return m_state->Backend().IsSubmissionComplete(token);
}

RhiSubmissionToken RenderDeviceServices::GetLastSubmittedToken(ERhiQueueType queueType) const noexcept
{
	return m_state->Backend().GetLastSubmittedToken(queueType);
}

void RenderDeviceServices::SubmitFrame() noexcept
{
	m_state->Backend().SubmitFrame();
}

void RenderDeviceServices::AdvanceFrameInFlight() noexcept
{
	m_state->Backend().AdvanceFrameInFlight();
}

void RenderDeviceServices::CloseExecuteAndFlushCurrentFrame() noexcept
{
	m_state->Backend().CloseExecuteAndFlushCurrentFrame();
}
