#include "../PCH.h"
#include "RayReconstruction/RayReconstructionSubsystem.h"

#include "RayReconstruction/NvidiaDlss/NvidiaDlssRayReconstructionProvider.h"

#include <format>
#include <utility>

namespace
{
	RayReconstructionProviderCapabilities BuildRendererCopyRayReconstructionCapabilities(
	    ERendererProviderCapabilityState state,
	    ERayReconstructionProviderFailureDomain failureDomain,
	    std::string reason)
	{
		return RayReconstructionProviderCapabilities{
		    .Kind = ERayReconstructionProviderKind::None,
		    .Category = ERendererProviderCategory::RayReconstruction,
		    .CapabilityState = state,
		    .FailureDomain = failureDomain,
		    .CanInitialize = true,
		    .CanEvaluate = true,
		    .UsesExternalSdk = false,
		    .ProviderName = "Renderer frame copy",
		    .ExternalRuntimeVersion = "none",
		    .RuntimeState = "RendererCopy",
		    .Reason = std::move(reason)};
	}
}

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
		m_diagnostics = BuildRendererCopyRayReconstructionCapabilities(
		    ERendererProviderCapabilityState::Enabled,
		    ERayReconstructionProviderFailureDomain::None,
		    "Ray reconstruction is disabled; final color is produced by the frame pass copy.");
		m_shutdown = false;
		return;
	}

	const bool initialized = m_activeProvider->Initialize(capabilities, m_nativeInterop, m_presentationBridge);
	if (!initialized && m_settings.Mode != EngineRayReconstructionMode::Off)
	{
		const RayReconstructionProviderCapabilities failedDiagnostics = m_activeProvider->GetDiagnostics();
		m_activeProvider->Shutdown();
		m_activeProvider.reset();
		m_diagnostics = BuildRendererCopyRayReconstructionCapabilities(
		    ERendererProviderCapabilityState::RuntimeFailed,
		    failedDiagnostics.FailureDomain,
		    std::format(
		        "Requested provider {} was unavailable: {} Final color is produced by the frame pass copy.",
		        RayReconstructionModeToString(m_settings.Mode),
		        failedDiagnostics.Reason));
	}
	else
	{
		RefreshDiagnostics(m_activeProvider.get());
	}

	m_shutdown = false;
}

void RayReconstructionSubsystem::SetupFrame(const RayReconstructionInputContract& inputContract)
{
	m_settings = BuildRayReconstructionSettingsFromCVars();
	m_lastInputValidation = ValidateRayReconstructionInputContract(inputContract);
	if (!m_lastInputValidation.Valid)
	{
		m_diagnostics = BuildRendererCopyRayReconstructionCapabilities(
		    ERendererProviderCapabilityState::RuntimeFailed,
		    ERayReconstructionProviderFailureDomain::InputContract,
		    std::format("Ray reconstruction input contract invalid: {}", m_lastInputValidation.Summary));
		m_diagnostics.RenderExtent = inputContract.RenderExtent;
		m_diagnostics.OutputExtent = inputContract.OutputExtent;
		m_diagnostics.ResetRequested = inputContract.ResetRequested;
		m_diagnostics.ResetReason = inputContract.ResetReason;
		m_diagnostics.ResourceContract = BuildRayReconstructionProviderResourceContract(inputContract);
		m_diagnostics.ResourceContractSummary = BuildProviderResourceContractSummary(m_diagnostics.ResourceContract);
		return;
	}

	if (m_activeProvider != nullptr)
	{
		m_activeProvider->SetupFrame(inputContract);
		RefreshDiagnostics(m_activeProvider.get());
		return;
	}

	m_diagnostics = BuildRendererCopyRayReconstructionCapabilities(
	    ERendererProviderCapabilityState::Enabled,
	    ERayReconstructionProviderFailureDomain::None,
	    "Ray reconstruction is disabled; final color is produced by the frame pass copy.");
	m_diagnostics.RenderExtent = inputContract.RenderExtent;
	m_diagnostics.OutputExtent = inputContract.OutputExtent;
	m_diagnostics.ResetRequested = inputContract.ResetRequested;
	m_diagnostics.ResetReason = inputContract.ResetReason;
	m_diagnostics.ResourceContract = BuildRayReconstructionProviderResourceContract(inputContract);
	m_diagnostics.ResourceContractSummary = BuildProviderResourceContractSummary(m_diagnostics.ResourceContract);
}

RayReconstructionEvaluationResult RayReconstructionSubsystem::Evaluate(const RayReconstructionEvaluationDesc& evaluation)
{
	if (!m_lastInputValidation.Valid)
	{
		return RayReconstructionEvaluationResult{
		    .ProducedOutput = false,
		    .FailureDomain = ERayReconstructionProviderFailureDomain::InputContract,
		    .Reason = std::format("Ray reconstruction input contract invalid: {}", m_lastInputValidation.Summary)};
	}

	if (m_activeProvider == nullptr)
	{
		return RayReconstructionEvaluationResult{
		    .ProducedOutput = false,
		    .FailureDomain = ERayReconstructionProviderFailureDomain::None,
		    .Reason = "Final color is produced by the frame pass copy."};
	}

	RayReconstructionEvaluationResult result = m_activeProvider->Evaluate(evaluation);
	RefreshDiagnostics(m_activeProvider.get());
	return result;
}

void RayReconstructionSubsystem::OnResize(RenderViewportExtent renderExtent, RenderViewportExtent outputExtent)
{
	if (m_activeProvider != nullptr)
	{
		m_activeProvider->OnResize(renderExtent, outputExtent);
		RefreshDiagnostics(m_activeProvider.get());
		return;
	}

	m_diagnostics.RenderExtent = renderExtent;
	m_diagnostics.OutputExtent = outputExtent;
}

void RayReconstructionSubsystem::ResetHistory(std::string_view reason)
{
	if (m_activeProvider != nullptr)
	{
		m_activeProvider->ResetHistory(reason);
		RefreshDiagnostics(m_activeProvider.get());
		return;
	}

	m_diagnostics.ResetRequested = true;
	m_diagnostics.ResetReason = std::string(reason);
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
	m_shutdown = true;
}

std::unique_ptr<IRayReconstructionProvider> RayReconstructionSubsystem::CreateProvider(EngineRayReconstructionMode mode)
{
	switch (mode)
	{
		case EngineRayReconstructionMode::NvidiaDlssRayReconstruction:
			return std::make_unique<NvidiaDlssRayReconstructionProvider>();
		case EngineRayReconstructionMode::Off:
			return {};
	}

	return {};
}
