#include "../PCH.h"
#include "Upscaling/UpscalerSubsystem.h"

#include "Upscaling/NvidiaDlss/NvidiaDlssUpscalerProvider.h"
#include "Upscaling/PassthroughUpscalerProvider.h"

#include <format>

namespace
{
	constexpr const char* BoolToString(bool value) noexcept
	{
		return value ? "true" : "false";
	}
}

UpscalerSubsystem::~UpscalerSubsystem() noexcept
{
	Shutdown();
}

void UpscalerSubsystem::Initialize(
    const RhiCapabilities& capabilities,
    RhiNativeDeviceQueueInterop nativeInterop,
    UpscalerPresentationBridge presentationBridge)
{
	m_nativeInterop = nativeInterop;
	m_presentationBridge = presentationBridge;
	m_settings = BuildUpscalerSettingsFromCVars();
	m_activeProvider = CreateProvider(m_settings.RequestedProvider);
	if (m_activeProvider == nullptr)
	{
		m_activeProvider = CreateFallbackProvider();
	}

	const bool initialized = m_activeProvider->Initialize(capabilities, m_nativeInterop, m_presentationBridge);
	if (!initialized && m_settings.RequestedProvider != EUpscalerProviderKind::Passthrough)
	{
		const UpscalerProviderCapabilities failedDiagnostics = m_activeProvider->GetDiagnostics();
		std::unique_ptr<IUpscalerProvider> fallback = CreateFallbackProvider();
		const bool fallbackInitialized = fallback->Initialize(capabilities, m_nativeInterop, m_presentationBridge);
		m_activeProvider = std::move(fallback);
		m_diagnostics = m_activeProvider->GetDiagnostics();
		m_diagnostics.Status = fallbackInitialized ? EUpscalerProviderStatus::FailedWithFallback : EUpscalerProviderStatus::Unavailable;
		m_diagnostics.FailureDomain = failedDiagnostics.FailureDomain;
		m_diagnostics.Reason = std::format(
		    "Requested provider {} was unavailable: {} Falling back to {}.",
		    UpscalerProviderKindToString(m_settings.RequestedProvider),
		    failedDiagnostics.Reason,
		    m_activeProvider->GetName());
	}
	else
	{
		m_diagnostics = m_activeProvider->GetDiagnostics();
	}

	m_frameFallbackProvider = CreateFallbackProvider();
	m_frameFallbackProvider->Initialize(capabilities, m_nativeInterop, m_presentationBridge);
	m_shutdown = false;
	const std::shared_ptr<spdlog::logger> logger = Logging::GetOrCreateLogger("Renderer.Upscaling");
	SPDLOG_LOGGER_INFO(
	    logger,
	    "Upscaler provider: requested={} active={} status={} failureDomain={} canEvaluate={} externalSdk={} runtimeVersion='{}' runtimeState='{}' "
	    "qualityMode='{}' featureMatrix='{}' renderExtent={}x{} outputExtent={}x{} resetRequested={} resetReason='{}' reason='{}'",
	    UpscalerProviderKindToString(m_settings.RequestedProvider),
	    m_activeProvider->GetName(),
	    UpscalerProviderStatusToString(m_diagnostics.Status),
	    UpscalerProviderFailureDomainToString(m_diagnostics.FailureDomain),
	    BoolToString(m_diagnostics.CanEvaluate),
	    BoolToString(m_diagnostics.UsesExternalSdk),
	    m_diagnostics.ExternalRuntimeVersion,
	    m_diagnostics.RuntimeState,
	    m_diagnostics.SelectedQualityMode,
	    m_diagnostics.FeatureMatrixSummary,
	    m_diagnostics.RenderExtent.Width,
	    m_diagnostics.RenderExtent.Height,
	    m_diagnostics.OutputExtent.Width,
	    m_diagnostics.OutputExtent.Height,
	    BoolToString(m_diagnostics.ResetRequested),
	    m_diagnostics.ResetReason,
	    m_diagnostics.Reason);
}

void UpscalerSubsystem::SetupFrame(const UpscalerInputContract& inputContract)
{
	m_lastInputValidation = ValidateUpscalerInputContract(inputContract);
	m_useFrameFallback = !m_lastInputValidation.Valid && m_activeProvider != nullptr &&
	                     m_activeProvider->GetKind() != EUpscalerProviderKind::Passthrough;
	m_frameFallbackReason.clear();

	const std::shared_ptr<spdlog::logger> logger = Logging::GetOrCreateLogger("Renderer.Upscaling");
	if (!m_lastInputValidation.Valid)
	{
		m_frameFallbackReason = std::format("Upscaler input contract invalid: {}", m_lastInputValidation.Summary);
		SPDLOG_LOGGER_WARN(logger, "{}. Using deterministic passthrough for this frame.", m_frameFallbackReason);
	}
	else if (m_settings.DiagnosticsEnabled)
	{
		SPDLOG_LOGGER_DEBUG(logger, "Upscaler input contract: {}", m_lastInputValidation.Summary);
	}

	IUpscalerProvider* const provider = m_useFrameFallback && m_frameFallbackProvider != nullptr ? m_frameFallbackProvider.get() : m_activeProvider.get();
	if (provider != nullptr)
	{
		provider->SetupFrame(inputContract);
	}
}

UpscalerEvaluationResult UpscalerSubsystem::Evaluate(const UpscalerEvaluationDesc& evaluation)
{
	IUpscalerProvider* const provider = m_useFrameFallback && m_frameFallbackProvider != nullptr ? m_frameFallbackProvider.get() : m_activeProvider.get();
	if (provider == nullptr)
	{
		return UpscalerEvaluationResult{
		    .ProducedOutput = false,
		    .UsedFallback = true,
		    .FailureDomain = EUpscalerProviderFailureDomain::Backend,
		    .Reason = "No upscaler provider is active."};
	}

	UpscalerEvaluationResult result = provider->Evaluate(evaluation);
	if (m_useFrameFallback)
	{
		result.UsedFallback = true;
		result.FailureDomain = EUpscalerProviderFailureDomain::InputContract;
		result.Reason = m_frameFallbackReason;
	}
	return result;
}

void UpscalerSubsystem::OnResize(RenderViewportExtent renderExtent, RenderViewportExtent outputExtent)
{
	if (m_activeProvider != nullptr)
	{
		m_activeProvider->OnResize(renderExtent, outputExtent);
	}
	if (m_frameFallbackProvider != nullptr)
	{
		m_frameFallbackProvider->OnResize(renderExtent, outputExtent);
	}
}

void UpscalerSubsystem::ResetHistory(std::string_view reason)
{
	if (m_activeProvider != nullptr)
	{
		m_activeProvider->ResetHistory(reason);
	}
	if (m_frameFallbackProvider != nullptr)
	{
		m_frameFallbackProvider->ResetHistory(reason);
	}
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
	if (m_frameFallbackProvider != nullptr)
	{
		m_frameFallbackProvider->Shutdown();
	}
	m_shutdown = true;
}

std::unique_ptr<IUpscalerProvider> UpscalerSubsystem::CreateProvider(EUpscalerProviderKind kind)
{
	switch (kind)
	{
		case EUpscalerProviderKind::Passthrough:
			return std::make_unique<PassthroughUpscalerProvider>();
		case EUpscalerProviderKind::NvidiaDlss:
			return std::make_unique<NvidiaDlssUpscalerProvider>();
	}

	return {};
}

std::unique_ptr<IUpscalerProvider> UpscalerSubsystem::CreateFallbackProvider()
{
	return std::make_unique<PassthroughUpscalerProvider>();
}
