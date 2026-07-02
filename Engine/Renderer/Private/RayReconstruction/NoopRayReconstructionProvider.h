#pragma once

#include "RayReconstruction/RayReconstructionProvider.h"

class NoopRayReconstructionProvider final : public IRayReconstructionProvider
{
  public:
	ERayReconstructionProviderKind GetKind() const noexcept override { return ERayReconstructionProviderKind::None; }
	std::string_view GetName() const noexcept override { return "No ray reconstruction"; }
	RayReconstructionProviderCapabilities QueryCapabilities(const RhiCapabilities& capabilities) const override;
	bool Initialize(
	    const RhiCapabilities& capabilities,
	    RhiNativeDeviceQueueInterop nativeInterop,
	    RayReconstructionPresentationBridge presentationBridge) override;
	void SetupFrame(const RayReconstructionInputContract& inputContract) override;
	RayReconstructionEvaluationResult Evaluate(const RayReconstructionEvaluationDesc& evaluation) override;
	void OnResize(RenderViewportExtent renderExtent, RenderViewportExtent outputExtent) override;
	void ResetHistory(std::string_view reason) override;
	void Shutdown() noexcept override {}
	RayReconstructionProviderCapabilities GetDiagnostics() const override { return m_diagnostics; }

  private:
	RayReconstructionProviderCapabilities m_diagnostics = {};
};
