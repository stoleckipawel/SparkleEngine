#pragma once

#include "RayReconstruction/NvidiaDlrr/StreamlineDlrrRuntime.h"
#include "RayReconstruction/RayReconstructionProvider.h"
#include "RayReconstruction/RayReconstructionSettings.h"

#include <memory>

class NvidiaDlrrProvider final : public IRayReconstructionProvider
{
  public:
	ERayReconstructionProviderKind GetKind() const noexcept override { return ERayReconstructionProviderKind::NvidiaDlrr; }
	std::string_view GetName() const noexcept override { return "NVIDIA DLRR"; }
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
	RayReconstructionInputContract m_lastInputContract = {};
	std::unique_ptr<IStreamlineDlrrRuntime> m_runtime;
	StreamlineDlrrRuntimeDiagnostics m_runtimeDiagnostics = {};
};
