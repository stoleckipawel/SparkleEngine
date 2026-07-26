#include "PCH.h"

#include "Device/RenderDeviceServices.h"
#include "Device/RenderDeviceBackendFactory.h"
#include "Device/RenderDeviceBackendServices.h"
#include "Device/RenderDeviceServicesState.h"
#include "Presentation/RhiPresentationDefaults.h"
#include "Shaders/CookedShaderPackageUtils.h"

#include "Window/Window.h"

#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Diagnostics/Verify.h"
#include <string>
#include <utility>

void RenderDeviceServices::FailCreation(std::string_view message) noexcept
{
	static const auto logger = Logging::GetOrCreateLogger("RHI.Services");
	Diagnostics::Fail(logger, __FILE__, __LINE__, message);
}

void RenderDeviceServices::FailUnsupportedBackend(ERhiBackendApi api) noexcept
{
	FailCreation(
	    std::string("RHI backend '") +
	    RhiBackendApiToString(api) +
	    "' is not available in this RenderDeviceServices build.");
}

void RenderDeviceServices::ValidateBackBufferFormat(PixelFormat backBufferFormat) noexcept
{
	if (!RhiPresentationDefaults::IsSupportedBackBufferFormat(backBufferFormat))
	{
		FailCreation(
		    std::string("Unsupported back buffer format for present swapchain: ") +
		    PixelFormatName(backBufferFormat));
	}
}

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
			FailUnsupportedBackend(backendApi);
			break;
		#endif
		case ERhiBackendApi::Vulkan:
		#if SPARKLE_RHI_WITH_VULKAN
			services->m_state->SetBackend(CreateVulkanRenderDeviceServices(window, backBufferFormat, externalFeatureHooks));
			break;
		#else
			FailUnsupportedBackend(backendApi);
			break;
		#endif
		case ERhiBackendApi::Unknown:
		default:
			FailUnsupportedBackend(backendApi);
	}

	return services;
}

const RhiCapabilities& RenderDeviceServices::GetCapabilities() const noexcept
{
	const RenderDeviceBackendServices& backend = m_state->GetBackend();
	return backend.GetRenderHardwareInterface().GetCapabilities();
}

RenderHardwareInterface& RenderDeviceServices::GetRenderHardwareInterface() noexcept
{
	return m_state->GetBackend().GetRenderHardwareInterface();
}

const RenderHardwareInterface& RenderDeviceServices::GetRenderHardwareInterface() const noexcept
{
	const RenderDeviceBackendServices& backend = m_state->GetBackend();
	return backend.GetRenderHardwareInterface();
}

RhiImGuiRenderer& RenderDeviceServices::GetImGuiRenderer() noexcept
{
	return m_state->GetBackend().GetImGuiRenderer();
}

void RenderDeviceServices::WaitForIdle() noexcept
{
	m_state->GetBackend().WaitForIdle();
}

void RenderDeviceServices::ResizeSwapChain() noexcept
{
	m_state->GetBackend().ResizeSwapChain();
}

void RenderDeviceServices::BeginFrame() noexcept
{
	RenderDeviceBackendServices& backend = m_state->GetBackend();
	backend.BeginFrame();
	RenderHardwareInterface& renderHardwareInterface = backend.GetRenderHardwareInterface();
	renderHardwareInterface.GetDescriptorService().BeginFrame(renderHardwareInterface.GetCurrentFrameIndex());
}

RenderCommandList& RenderDeviceServices::GetCurrentGraphicsCommandList() noexcept
{
	return m_state->GetBackend().GetCurrentGraphicsCommandList();
}

RenderCommandList& RenderDeviceServices::GetGraphicsCommandList(std::uint32_t frameIndex) noexcept
{
	return m_state->GetBackend().GetGraphicsCommandList(frameIndex);
}

RenderCommandList& RenderDeviceServices::BeginCurrentGraphicsCommandList() noexcept
{
	return m_state->GetBackend().BeginCurrentGraphicsCommandList();
}

RhiCommandRecordingLease RenderDeviceServices::AcquireCommandRecordingLease(
    ERhiQueueType queueType,
    RhiCommandRecordingOwner owner) noexcept
{
	return m_state->GetBackend().AcquireCommandRecordingLease(queueType, owner);
}

RhiSubmissionToken RenderDeviceServices::SubmitCommandRecordingLease(
    RhiCommandRecordingLease&& lease,
    std::span<const RhiSubmissionToken> waitTokens) noexcept
{
	return m_state->GetBackend().SubmitCommandRecordingLease(std::move(lease), waitTokens);
}

RhiSubmissionToken RenderDeviceServices::SubmitCurrentGraphicsCommandList(
    std::span<const RhiSubmissionToken> waitTokens) noexcept
{
	return m_state->GetBackend().SubmitCurrentGraphicsCommandList(waitTokens);
}

void RenderDeviceServices::QueueWait(ERhiQueueType waitQueue, RhiSubmissionToken executionToken) noexcept
{
	m_state->GetBackend().QueueWait(waitQueue, executionToken);
}

void RenderDeviceServices::WaitForSubmission(RhiSubmissionToken token) noexcept
{
	m_state->GetBackend().WaitForSubmission(token);
}

bool RenderDeviceServices::IsSubmissionComplete(RhiSubmissionToken token) const noexcept
{
	return m_state->GetBackend().IsSubmissionComplete(token);
}

RhiSubmissionToken RenderDeviceServices::GetLastSubmittedToken(ERhiQueueType queueType) const noexcept
{
	return m_state->GetBackend().GetLastSubmittedToken(queueType);
}

void RenderDeviceServices::SubmitFrame() noexcept
{
	m_state->GetBackend().SubmitFrame();
}

void RenderDeviceServices::AdvanceFrameInFlight() noexcept
{
	m_state->GetBackend().AdvanceFrameInFlight();
}

void RenderDeviceServices::CloseExecuteAndFlushCurrentFrame() noexcept
{
	m_state->GetBackend().CloseExecuteAndFlushCurrentFrame();
}
