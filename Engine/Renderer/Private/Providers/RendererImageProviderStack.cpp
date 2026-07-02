#include "../PCH.h"
#include "Providers/RendererImageProviderStack.h"

#include "RayReconstruction/RayReconstructionSettings.h"
#include "RayReconstruction/RayReconstructionSubsystem.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Upscaling/UpscalerProvider.h"
#include "Upscaling/UpscalerSettings.h"
#include "Upscaling/UpscalerSubsystem.h"

namespace
{
	bool UpgradePresentationInterfaceThroughRhi(
	    RhiNativeInterfaceUpgradeCallback callback,
	    void* callbackUserData,
	    void* bridgeUserData)
	{
		RenderHardwareInterface* const hardware = static_cast<RenderHardwareInterface*>(bridgeUserData);
		return hardware != nullptr && hardware->GetInteropService().UpgradePresentationInterface(callback, callbackUserData);
	}

	UpscalerSettings BuildImageProviderUpscalerSettings()
	{
		UpscalerSettings settings = BuildUpscalerSettingsFromCVars();
		if (GetRayReconstructionModeFromCVars() != EngineRayReconstructionMode::Off)
		{
			settings.RequestedProvider = EUpscalerProviderKind::Passthrough;
		}
		return settings;
	}
}

RendererImageProviderStack::RendererImageProviderStack() = default;

RendererImageProviderStack::~RendererImageProviderStack() noexcept
{
	Shutdown();
}

void RendererImageProviderStack::Initialize(RenderHardwareInterface& renderHardware)
{
	m_upscalerSubsystem = std::make_unique<UpscalerSubsystem>();
	m_upscalerSubsystem->Initialize(
	    renderHardware.GetCapabilities(),
	    renderHardware.GetInteropService().GetDeviceQueueInterop(
	        RhiNativeInteropRequest{
	            .Consumer = ERhiNativeInteropConsumer::UpscalerProvider,
	            .Reason = "Renderer upscaler provider initialization"}),
	    BuildImageProviderUpscalerSettings(),
	    UpscalerPresentationBridge{
	        .UpgradePresentationInterface = &UpgradePresentationInterfaceThroughRhi,
	        .UserData = &renderHardware});

	m_rayReconstructionSubsystem = std::make_unique<RayReconstructionSubsystem>();
	m_rayReconstructionSubsystem->Initialize(
	    renderHardware.GetCapabilities(),
	    renderHardware.GetInteropService().GetDeviceQueueInterop(
	        RhiNativeInteropRequest{
	            .Consumer = ERhiNativeInteropConsumer::RayReconstructionProvider,
	            .Reason = "Renderer ray reconstruction provider initialization"}),
	    RayReconstructionPresentationBridge{
	        .UpgradePresentationInterface = &UpgradePresentationInterfaceThroughRhi,
	        .UserData = &renderHardware});
}

void RendererImageProviderStack::Refresh(RenderHardwareInterface& renderHardware)
{
	Shutdown();
	Initialize(renderHardware);
}

void RendererImageProviderStack::Shutdown() noexcept
{
	if (m_upscalerSubsystem != nullptr)
	{
		m_upscalerSubsystem->Shutdown();
		m_upscalerSubsystem.reset();
	}
	if (m_rayReconstructionSubsystem != nullptr)
	{
		m_rayReconstructionSubsystem->Shutdown();
		m_rayReconstructionSubsystem.reset();
	}
}

void RendererImageProviderStack::OnResize(RenderViewportExtent renderExtent, RenderViewportExtent outputExtent)
{
	if (m_upscalerSubsystem != nullptr)
	{
		m_upscalerSubsystem->OnResize(renderExtent, outputExtent);
	}
	if (m_rayReconstructionSubsystem != nullptr)
	{
		m_rayReconstructionSubsystem->OnResize(renderExtent, outputExtent);
	}
}

void RendererImageProviderStack::ResetHistory(std::string_view reason)
{
	if (m_upscalerSubsystem != nullptr)
	{
		m_upscalerSubsystem->ResetHistory(reason);
	}
	if (m_rayReconstructionSubsystem != nullptr)
	{
		m_rayReconstructionSubsystem->ResetHistory(reason);
	}
}

void RendererImageProviderStack::SetupUpscalerFrame(const UpscalerInputContract& inputContract)
{
	if (m_upscalerSubsystem != nullptr)
	{
		m_upscalerSubsystem->SetupFrame(inputContract);
	}
}

void RendererImageProviderStack::SetupRayReconstructionFrame(const RayReconstructionInputContract& inputContract)
{
	if (m_rayReconstructionSubsystem != nullptr)
	{
		m_rayReconstructionSubsystem->SetupFrame(inputContract);
	}
}

std::uint32_t RendererImageProviderStack::GetFrameGraphKey() const noexcept
{
	return static_cast<std::uint32_t>(GetRayReconstructionModeFromCVars());
}

RendererImageProviderPassServices RendererImageProviderStack::BuildPassServices() noexcept
{
	return RendererImageProviderPassServices{
	    .Upscaling = RenderUpscalingPassServices{.Subsystem = m_upscalerSubsystem.get()},
	    .RayReconstruction = RenderRayReconstructionPassServices{.Subsystem = m_rayReconstructionSubsystem.get()}};
}

RendererProviderDiagnosticsSnapshot RendererImageProviderStack::CaptureUpscalerDiagnosticsSnapshot() const
{
	if (m_upscalerSubsystem == nullptr)
	{
		return RendererProviderDiagnosticsSnapshot{};
	}

	const UpscalerProviderCapabilities& provider = m_upscalerSubsystem->GetDiagnostics();
	return RendererProviderDiagnosticsSnapshot{
	    .Status = ERendererDiagnosticStatus::Available,
	    .RequestedProvider = UpscalerProviderKindToString(m_upscalerSubsystem->GetRequestedProviderKind()),
	    .ActiveProvider = provider.ProviderName.empty() ? std::string(m_upscalerSubsystem->GetActiveProvider().GetName()) :
	                                                      provider.ProviderName,
	    .Category = RendererProviderCategoryToString(provider.Category),
	    .CapabilityState = RendererProviderCapabilityStateToString(provider.CapabilityState),
	    .FailureDomain = UpscalerProviderFailureDomainToString(provider.FailureDomain),
	    .CanEvaluate = provider.CanEvaluate,
	    .UsesExternalSdk = provider.UsesExternalSdk,
	    .RuntimeVersion = provider.ExternalRuntimeVersion,
	    .RuntimeState = provider.RuntimeState,
	    .ResourceContract = provider.ResourceContractSummary,
	    .Reason = provider.Reason};
}
