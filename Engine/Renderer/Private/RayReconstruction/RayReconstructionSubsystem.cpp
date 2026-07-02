#include "../PCH.h"
#include "RayReconstruction/RayReconstructionSubsystem.h"

#include "RayReconstruction/NoopRayReconstructionProvider.h"
#include "RayReconstruction/NvidiaDlss/NvidiaDlssRayReconstructionProvider.h"

#include <format>

RayReconstructionSubsystem::~RayReconstructionSubsystem() noexcept
{
	Shutdown();
}

void RayReconstructionSubsystem::RefreshDiagnostics(IRayReconstructionProvider* provider) noexcept
{
	if (provider != nullptr)
	{
		m_diagnostics = provider->GetDiagnostics();
	}
}

void RayReconstructionSubsystem::Initialize(
    const RhiCapabilities& capabilities,
    RhiNativeDeviceQueueInterop nativeInterop,
    RayReconstructionPresentationBridge presentationBridge)
{
	m_nativeInterop = nativeInterop;
	m_presentationBridge = presentationBridge;
	m_settings = BuildRayReconstructionSettingsFromCVars();
	m_activeProvider = CreateProvider(m_settings.Mode);
	if (m_activeProvider == nullptr)
	{
		m_activeProvider = CreateFallbackProvider();
	}

	const bool initialized = m_activeProvider->Initialize(capabilities, m_nativeInterop, m_presentationBridge);
	if (!initialized && m_settings.Mode != EngineRayReconstructionMode::Off)
	{
		const RayReconstructionProviderCapabilities failedDiagnostics = m_activeProvider->GetDiagnostics();
		std::unique_ptr<IRayReconstructionProvider> fallback = CreateFallbackProvider();
		const bool fallbackInitialized = fallback->Initialize(capabilities, m_nativeInterop, m_presentationBridge);
		m_activeProvider = std::move(fallback);
		RefreshDiagnostics(m_activeProvider.get());
		m_diagnostics.CapabilityState =
		    fallbackInitialized ? ERendererProviderCapabilityState::RuntimeFailed : ERendererProviderCapabilityState::Unavailable;
		m_diagnostics.FailureDomain = failedDiagnostics.FailureDomain;
		m_diagnostics.Reason = std::format(
		    "Requested provider {} was unavailable: {} Falling back to {}.",
		    RayReconstructionModeToString(m_settings.Mode),
		    failedDiagnostics.Reason,
		    m_activeProvider->GetName());
	}
	else
	{
		RefreshDiagnostics(m_activeProvider.get());
	}

	m_frameFallbackProvider = CreateFallbackProvider();
	m_frameFallbackProvider->Initialize(capabilities, m_nativeInterop, m_presentationBridge);
	m_shutdown = false;
}

void RayReconstructionSubsystem::SetupFrame(const RayReconstructionInputContract& inputContract)
{
	m_settings = BuildRayReconstructionSettingsFromCVars();
	m_lastInputValidation = ValidateRayReconstructionInputContract(inputContract);
	m_useFrameFallback = !m_lastInputValidation.Valid && m_activeProvider != nullptr &&
	                     m_activeProvider->GetKind() != ERayReconstructionProviderKind::None;
	m_frameFallbackReason.clear();

	if (!m_lastInputValidation.Valid)
	{
		m_frameFallbackReason = std::format("Ray reconstruction input contract invalid: {}", m_lastInputValidation.Summary);
	}

	IRayReconstructionProvider* const provider =
	    m_useFrameFallback && m_frameFallbackProvider != nullptr ? m_frameFallbackProvider.get() : m_activeProvider.get();
	if (provider != nullptr)
	{
		provider->SetupFrame(inputContract);
		RefreshDiagnostics(provider);
	}
}

RayReconstructionEvaluationResult RayReconstructionSubsystem::Evaluate(const RayReconstructionEvaluationDesc& evaluation)
{
	IRayReconstructionProvider* const provider =
	    m_useFrameFallback && m_frameFallbackProvider != nullptr ? m_frameFallbackProvider.get() : m_activeProvider.get();
	if (provider == nullptr)
	{
		return RayReconstructionEvaluationResult{
		    .ProducedOutput = false,
		    .UsedFallback = true,
		    .FailureDomain = ERayReconstructionProviderFailureDomain::Backend,
		    .Reason = "No ray reconstruction provider is active."};
	}

	RayReconstructionEvaluationResult result = provider->Evaluate(evaluation);
	RefreshDiagnostics(provider);
	if (m_useFrameFallback)
	{
		result.UsedFallback = true;
		result.FailureDomain = ERayReconstructionProviderFailureDomain::InputContract;
		result.Reason = m_frameFallbackReason;
		m_diagnostics.CapabilityState = ERendererProviderCapabilityState::RuntimeFailed;
		m_diagnostics.FailureDomain = result.FailureDomain;
		m_diagnostics.Reason = result.Reason;
	}
	return result;
}

void RayReconstructionSubsystem::OnResize(RenderViewportExtent renderExtent, RenderViewportExtent outputExtent)
{
	if (m_activeProvider != nullptr)
	{
		m_activeProvider->OnResize(renderExtent, outputExtent);
		RefreshDiagnostics(m_activeProvider.get());
	}
	if (m_frameFallbackProvider != nullptr)
	{
		m_frameFallbackProvider->OnResize(renderExtent, outputExtent);
	}
}

void RayReconstructionSubsystem::ResetHistory(std::string_view reason)
{
	if (m_activeProvider != nullptr)
	{
		m_activeProvider->ResetHistory(reason);
		RefreshDiagnostics(m_activeProvider.get());
	}
	if (m_frameFallbackProvider != nullptr)
	{
		m_frameFallbackProvider->ResetHistory(reason);
	}
}

void RayReconstructionSubsystem::Shutdown() noexcept
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

std::unique_ptr<IRayReconstructionProvider> RayReconstructionSubsystem::CreateProvider(EngineRayReconstructionMode mode)
{
	switch (mode)
	{
		case EngineRayReconstructionMode::NvidiaDlssRayReconstruction:
			return std::make_unique<NvidiaDlssRayReconstructionProvider>();
		case EngineRayReconstructionMode::Off:
			return std::make_unique<NoopRayReconstructionProvider>();
	}

	return {};
}

std::unique_ptr<IRayReconstructionProvider> RayReconstructionSubsystem::CreateFallbackProvider()
{
	return std::make_unique<NoopRayReconstructionProvider>();
}
