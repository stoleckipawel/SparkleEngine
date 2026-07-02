#pragma once

#include "RayReconstruction/NvidiaDlss/StreamlineDlssRayReconstructionRuntime.h"
#include "RayReconstruction/RayReconstructionProvider.h"
#include "RayReconstruction/RayReconstructionSettings.h"

#include <memory>

class NvidiaDlssRayReconstructionProvider final : public IRayReconstructionProvider
{
  public:
	ERayReconstructionProviderKind GetKind() const noexcept override { return ERayReconstructionProviderKind::NvidiaDlssRayReconstruction; }
	std::string_view GetName() const noexcept override { return "NVIDIA DLSS Ray Reconstruction"; }
	RayReconstructionProviderCapabilities QueryCapabilities(const RhiCapabilities& capabilities) const override;
	bool Initialize(
	    const RhiCapabilities& capabilities,
	    RhiNativeDeviceQueueInterop nativeInterop,
	    RayReconstructionPresentationBridge presentationBridge) override;
	void SetupFrame(const RayReconstructionInputContract& inputContract) override;
	RayReconstructionEvaluationResult Evaluate(const RayReconstructionEvaluationDesc& evaluation) override;
	void OnResize(RenderViewportExtent renderExtent, RenderViewportExtent outputExtent) override;
	void ResetHistory(std::string_view reason) override;
	void Shutdown() noexcept override;
	RayReconstructionProviderCapabilities GetDiagnostics() const override;

  private:
	RayReconstructionProviderCapabilities m_diagnostics = {};
	RayReconstructionInputContract m_lastInputContract = {};
	std::unique_ptr<IStreamlineDlssRayReconstructionRuntime> m_runtime;
	EngineRayReconstructionQualityMode m_qualityMode = EngineRayReconstructionQualityMode::Quality;
	StreamlineDlssRayReconstructionRuntimeDiagnostics m_runtimeDiagnostics = {};
};
