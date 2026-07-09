#include "../PCH.h"
#include "Upscaling/UpscalerSubsystem.h"

#include "Upscaling/NvidiaDlss/NvidiaDlssUpscalerProvider.h"

#include <format>
#include <utility>

namespace
{
	UpscalerProviderCapabilities BuildRendererCopyUpscalerCapabilities(
	    ERendererProviderCapabilityState state,
	    EUpscalerProviderFailureDomain failureDomain,
	    std::string reason)
	{
		return UpscalerProviderCapabilities{
		    .Kind = EUpscalerProviderKind::Linear,
		    .Category = ERendererProviderCategory::Upscaler,
		    .CapabilityState = state,
		    .FailureDomain = failureDomain,
		    .CanInitialize = true,
		    .CanEvaluate = true,
		    .UsesExternalSdk = false,
		    .ProviderName = "Renderer linear upscaler",
		    .Reason = std::move(reason)};
	}
}

UpscalerSubsystem::~UpscalerSubsystem() noexcept
{
	Shutdown();
}

void UpscalerSubsystem::RefreshDiagnostics(IUpscalerProvider* provider) noexcept
{
	if (provider != nullptr)
	{
		m_diagnostics = provider->GetDiagnostics();
	}
}

void UpscalerSubsystem::Initialize(
    const RhiCapabilities& capabilities,
    RhiNativeDeviceQueueInterop nativeInterop,
    const UpscalerSettings& settings,
    UpscalerPresentationBridge presentationBridge)
{
	m_nativeInterop = nativeInterop;
	m_presentationBridge = presentationBridge;
	m_settings = settings;
	m_activeProvider = CreateProvider(m_settings.RequestedProvider);
	if (m_activeProvider == nullptr)
	{
		m_diagnostics = BuildRendererCopyUpscalerCapabilities(
		    ERendererProviderCapabilityState::Enabled,
		    EUpscalerProviderFailureDomain::None,
		    "Linear upscaler selected; final color is produced by the renderer linear upscale pass.");
		m_shutdown = false;
		return;
	}

	const bool initialized = m_activeProvider->Initialize(capabilities, m_nativeInterop, m_presentationBridge);
	if (!initialized)
	{
		const UpscalerProviderCapabilities failedDiagnostics = m_activeProvider->GetDiagnostics();
		m_activeProvider->Shutdown();
		m_activeProvider.reset();
		m_diagnostics = BuildRendererCopyUpscalerCapabilities(
		    ERendererProviderCapabilityState::RuntimeFailed,
		    failedDiagnostics.FailureDomain,
		    std::format(
		        "Requested provider {} was unavailable: {} Final color is produced by the renderer linear upscale pass.",
		        UpscalerProviderKindToString(m_settings.RequestedProvider),
		        failedDiagnostics.Reason));
	}
	else
	{
		RefreshDiagnostics(m_activeProvider.get());
	}

	m_shutdown = false;
}

void UpscalerSubsystem::SetupFrame(const UpscalerInputContract& inputContract)
{
	m_lastInputValidation = ValidateUpscalerInputContract(inputContract);
	if (!m_lastInputValidation.Valid)
	{
		m_diagnostics = BuildRendererCopyUpscalerCapabilities(
		    ERendererProviderCapabilityState::RuntimeFailed,
		    EUpscalerProviderFailureDomain::InputContract,
		    std::format("Upscaler input contract invalid: {}", m_lastInputValidation.Summary));
		m_diagnostics.ResourceContract = BuildUpscalerProviderResourceContract(inputContract);
		return;
	}

	if (m_activeProvider != nullptr)
	{
		m_activeProvider->SetupFrame(inputContract);
		RefreshDiagnostics(m_activeProvider.get());
		return;
	}

	m_diagnostics = BuildRendererCopyUpscalerCapabilities(
	    ERendererProviderCapabilityState::Enabled,
	    EUpscalerProviderFailureDomain::None,
	    "Linear upscaler selected; final color is produced by the renderer linear upscale pass.");
	m_diagnostics.ResourceContract = BuildUpscalerProviderResourceContract(inputContract);
}

UpscalerEvaluationResult UpscalerSubsystem::Evaluate(const UpscalerEvaluationDesc& evaluation)
{
	if (!m_lastInputValidation.Valid)
	{
		return UpscalerEvaluationResult{
		    .ProducedOutput = false,
		    .FailureDomain = EUpscalerProviderFailureDomain::InputContract,
		    .Reason = std::format("Upscaler input contract invalid: {}", m_lastInputValidation.Summary)};
	}

	if (m_activeProvider == nullptr)
	{
		return UpscalerEvaluationResult{
		    .ProducedOutput = false,
		    .FailureDomain = EUpscalerProviderFailureDomain::None,
		    .Reason = "Final color is produced by the renderer linear upscale pass."};
	}

	UpscalerEvaluationResult result = m_activeProvider->Evaluate(evaluation);
	RefreshDiagnostics(m_activeProvider.get());
	return result;
}

void UpscalerSubsystem::OnResize(RenderViewportExtent renderExtent, RenderViewportExtent outputExtent)
{
	if (m_activeProvider != nullptr)
	{
		m_activeProvider->OnResize(renderExtent, outputExtent);
		RefreshDiagnostics(m_activeProvider.get());
		return;
	}

	(void) renderExtent;
	(void) outputExtent;
}

void UpscalerSubsystem::ResetHistory(std::string_view reason)
{
	if (m_activeProvider != nullptr)
	{
		m_activeProvider->ResetHistory(reason);
		RefreshDiagnostics(m_activeProvider.get());
		return;
	}

	(void) reason;
}

void UpscalerSubsystem::Shutdown() noexcept
{
	if (m_shutdown)
	{
		return;
	}

	if (m_activeProvider != nullptr)
	{
		m_activeProvider->Shutdown();
	}
	m_shutdown = true;
}

std::unique_ptr<IUpscalerProvider> UpscalerSubsystem::CreateProvider(EUpscalerProviderKind kind)
{
	switch (kind)
	{
		case EUpscalerProviderKind::Linear:
			return {};
		case EUpscalerProviderKind::NvidiaDlss:
			return std::make_unique<NvidiaDlssUpscalerProvider>();
	}

	return {};
}
