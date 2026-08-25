#include "PCH.h"

#include "Device/RenderDeviceServices.h"
#include "Device/RenderDeviceBackendFactory.h"
#include "Device/RenderDeviceBackendServices.h"
#include "Device/RenderDeviceServicesState.h"
#include "CVars/RHICVars.h"
#include "Presentation/RhiPresentationDefaults.h"

#include "Window/Window.h"

#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Diagnostics/Verify.h"
#include <format>
#include <string>
#include <utility>

void RenderDeviceServices::FailCreation(std::string_view message) noexcept
{
	static const auto logger = Logging::GetOrCreateLogger("RHI.Services");
	Diagnostics::Fatal(logger, __FILE__, __LINE__, message);
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

RhiPresentationConfiguration RenderDeviceServices::ResolvePresentationConfiguration() noexcept
{
	const RhiPresentationConfiguration configuration{
	    .BackBufferCount = CVarBackBufferCount.Get(),
	    .MaximumFramesInFlight = CVarMaximumFramesInFlight.Get()};
	if (configuration.BackBufferCount < RhiPresentationDefaults::MinBackBufferCount ||
	    configuration.BackBufferCount > RhiPresentationDefaults::MaxBackBufferCount)
	{
		FailCreation(std::format(
		    "r.BackBufferCount={} is outside the DXGI flip-model and Sparkle presentation range [2, 3]. Use r.MaximumFramesInFlight=1 for single-frame pacing.",
		    configuration.BackBufferCount));
	}
	if (configuration.MaximumFramesInFlight < RhiPresentationDefaults::MinFramesInFlight ||
	    configuration.MaximumFramesInFlight > RhiPresentationDefaults::MaxFramesInFlight)
	{
		FailCreation(std::format(
		    "r.MaximumFramesInFlight={} is outside the supported range [1, 3].",
		    configuration.MaximumFramesInFlight));
	}
	if (configuration.MaximumFramesInFlight > configuration.BackBufferCount)
	{
		FailCreation(std::format(
		    "r.MaximumFramesInFlight={} exceeds r.BackBufferCount={}.",
		    configuration.MaximumFramesInFlight,
		    configuration.BackBufferCount));
	}
	return configuration;
}

RenderDeviceServices::RenderDeviceServices() noexcept : m_state(std::make_unique<RenderDeviceServicesState>()) {}

RenderDeviceServices::~RenderDeviceServices() noexcept = default;

std::unique_ptr<RenderDeviceServices> RenderDeviceServices::Create(Window& window) noexcept
{
	return Create(window, ResolveDefaultRhiBackendApi(), RhiPresentationDefaults::DefaultBackBufferFormat);
}

std::unique_ptr<RenderDeviceServices> RenderDeviceServices::Create(Window& window, ERhiBackendApi backendApi) noexcept
{
	return Create(window, backendApi, RhiPresentationDefaults::DefaultBackBufferFormat);
}

std::unique_ptr<RenderDeviceServices> RenderDeviceServices::Create(
    Window& window,
    ERhiBackendApi backendApi,
    RhiInterposerHooks interposerHooks) noexcept
{
	return Create(window, backendApi, CVarBackBufferFormat.Get(), interposerHooks);
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
    RhiInterposerHooks interposerHooks) noexcept
{
	ValidateBackBufferFormat(backBufferFormat);
	const RhiPresentationConfiguration presentationConfiguration = ResolvePresentationConfiguration();

	auto services = std::unique_ptr<RenderDeviceServices>(new RenderDeviceServices());
	switch (backendApi)
	{
		case ERhiBackendApi::D3D12:
		#if SPARKLE_RHI_WITH_D3D12
			services->m_state->SetBackendServices(
			    CreateD3D12RenderDeviceServices(window, backBufferFormat, presentationConfiguration, interposerHooks));
			break;
		#else
			FailUnsupportedBackend(backendApi);
			break;
		#endif
		case ERhiBackendApi::Vulkan:
		#if SPARKLE_RHI_WITH_VULKAN
			if (static_cast<bool>(interposerHooks))
			{
				FailCreation("Vulkan device creation received interposer hooks that only the D3D12 bridge implements.");
			}
			services->m_state->SetBackendServices(
			    CreateVulkanRenderDeviceServices(window, backBufferFormat, presentationConfiguration));
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
	const RenderDeviceBackendServices& backendServices = m_state->GetBackendServices();
	return backendServices.GetRenderHardwareInterface().GetCapabilities();
}

RenderHardwareInterface& RenderDeviceServices::GetRenderHardwareInterface() noexcept
{
	return m_state->GetBackendServices().GetRenderHardwareInterface();
}

const RenderHardwareInterface& RenderDeviceServices::GetRenderHardwareInterface() const noexcept
{
	const RenderDeviceBackendServices& backendServices = m_state->GetBackendServices();
	return backendServices.GetRenderHardwareInterface();
}

RhiImGuiRenderer& RenderDeviceServices::GetImGuiRenderer() noexcept
{
	return m_state->GetBackendServices().GetImGuiRenderer();
}

void RenderDeviceServices::SettleForShutdown() noexcept
{
	m_state->GetBackendServices().SettleForShutdown();
}

void RenderDeviceServices::ResizeSwapChain() noexcept
{
	m_state->GetBackendServices().ResizeSwapChain();
}

void RenderDeviceServices::BeginFrame(std::uint64_t frameId) noexcept
{
	RenderDeviceBackendServices& backendServices = m_state->GetBackendServices();
	backendServices.BeginFrame(frameId);
	RenderHardwareInterface& renderHardwareInterface = backendServices.GetRenderHardwareInterface();
	renderHardwareInterface.GetDescriptorService().BeginFrame(renderHardwareInterface.GetCurrentFrameIndex());
}

void RenderDeviceServices::PrepareCommandRecording() noexcept
{
	m_state->GetBackendServices().PrepareCommandRecording();
}

RenderCommandList& RenderDeviceServices::GetCurrentGraphicsCommandList() noexcept
{
	return m_state->GetBackendServices().GetCurrentGraphicsCommandList();
}

RenderCommandList& RenderDeviceServices::GetGraphicsCommandList(std::uint32_t frameIndex) noexcept
{
	return m_state->GetBackendServices().GetGraphicsCommandList(frameIndex);
}

RenderCommandList& RenderDeviceServices::BeginCurrentGraphicsCommandList() noexcept
{
	return m_state->GetBackendServices().BeginCurrentGraphicsCommandList();
}

RhiCommandRecordingLease RenderDeviceServices::AcquireCommandRecordingLease(
    ERhiQueueType queueType,
    RhiCommandRecordingOwner owner) noexcept
{
	return m_state->GetBackendServices().AcquireCommandRecordingLease(queueType, owner);
}

RhiCommandRecordingLease RenderDeviceServices::TakeCurrentGraphicsCommandRecordingLease() noexcept
{
	return m_state->GetBackendServices().TakeCurrentGraphicsCommandRecordingLease();
}

RhiSubmissionToken RenderDeviceServices::SubmitCommandRecordingLease(
    RhiCommandRecordingLease&& lease,
    std::span<const RhiSubmissionToken> waitTokens) noexcept
{
	return m_state->GetBackendServices().SubmitCommandRecordingLease(std::move(lease), waitTokens);
}

RhiSubmissionToken RenderDeviceServices::SubmitCommandRecordingBatch(
    std::span<RhiCommandRecordingLease> leases,
    std::span<const RhiSubmissionToken> waitTokens) noexcept
{
	return m_state->GetBackendServices().SubmitCommandRecordingBatch(leases, waitTokens);
}

RhiSubmissionToken RenderDeviceServices::SubmitCurrentGraphicsCommandList(
    std::span<const RhiSubmissionToken> waitTokens) noexcept
{
	return m_state->GetBackendServices().SubmitCurrentGraphicsCommandList(waitTokens);
}

void RenderDeviceServices::QueueWait(ERhiQueueType waitQueue, RhiSubmissionToken executionToken) noexcept
{
	m_state->GetBackendServices().QueueWait(waitQueue, executionToken);
}

void RenderDeviceServices::WaitForSubmission(RhiSubmissionToken token) noexcept
{
	m_state->GetBackendServices().WaitForSubmission(token);
}

bool RenderDeviceServices::IsSubmissionComplete(RhiSubmissionToken token) const noexcept
{
	return m_state->GetBackendServices().IsSubmissionComplete(token);
}

RhiSubmissionToken RenderDeviceServices::GetLastSubmittedToken(ERhiQueueType queueType) const noexcept
{
	return m_state->GetBackendServices().GetLastSubmittedToken(queueType);
}

void RenderDeviceServices::SubmitFrame(std::uint64_t frameId) noexcept
{
	m_state->GetBackendServices().SubmitFrame(frameId);
}

void RenderDeviceServices::AdvanceFrameInFlight() noexcept
{
	m_state->GetBackendServices().AdvanceFrameInFlight();
}
