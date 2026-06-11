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

void UpscalerSubsystem::Initialize(const RhiCapabilities& capabilities)
{
	m_settings = BuildUpscalerSettingsFromCVars();
	m_activeProvider = CreateProvider(m_settings.RequestedProvider);
	if (m_activeProvider == nullptr)
	{
		m_activeProvider = CreateFallbackProvider();
	}

	const bool initialized = m_activeProvider->Initialize(capabilities);
	if (!initialized && m_settings.RequestedProvider != EUpscalerProviderKind::Passthrough)
	{
		const UpscalerProviderCapabilities failedDiagnostics = m_activeProvider->GetDiagnostics();
		std::unique_ptr<IUpscalerProvider> fallback = CreateFallbackProvider();
		const bool fallbackInitialized = fallback->Initialize(capabilities);
		m_activeProvider = std::move(fallback);
		m_diagnostics = m_activeProvider->GetDiagnostics();
		m_diagnostics.Status = fallbackInitialized ? EUpscalerProviderStatus::FailedWithFallback : EUpscalerProviderStatus::Unavailable;
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

	m_shutdown = false;
	const std::shared_ptr<spdlog::logger> logger = Logging::GetOrCreateLogger("Renderer.Upscaling");
	SPDLOG_LOGGER_INFO(
	    logger,
	    "Upscaler provider: requested={} active={} status={} canEvaluate={} externalSdk={} reason='{}'",
	    UpscalerProviderKindToString(m_settings.RequestedProvider),
	    m_activeProvider->GetName(),
	    UpscalerProviderStatusToString(m_diagnostics.Status),
	    BoolToString(m_diagnostics.CanEvaluate),
	    BoolToString(m_diagnostics.UsesExternalSdk),
	    m_diagnostics.Reason);
}

void UpscalerSubsystem::SetupFrame(const UpscalerFrameSetupDesc& frameSetup)
{
	if (m_activeProvider != nullptr)
	{
		m_activeProvider->SetupFrame(frameSetup);
	}
}

UpscalerEvaluationResult UpscalerSubsystem::Evaluate(const UpscalerEvaluationDesc& evaluation)
{
	if (m_activeProvider == nullptr)
	{
		return UpscalerEvaluationResult{
		    .ProducedOutput = false,
		    .UsedFallback = true,
		    .Reason = "No upscaler provider is active."};
	}

	return m_activeProvider->Evaluate(evaluation);
}

void UpscalerSubsystem::OnResize(RenderViewportExtent renderExtent, RenderViewportExtent outputExtent)
{
	if (m_activeProvider != nullptr)
	{
		m_activeProvider->OnResize(renderExtent, outputExtent);
	}
}

void UpscalerSubsystem::ResetHistory(std::string_view reason)
{
	if (m_activeProvider != nullptr)
	{
		m_activeProvider->ResetHistory(reason);
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
