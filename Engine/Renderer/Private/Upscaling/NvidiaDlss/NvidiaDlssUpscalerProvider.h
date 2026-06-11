#pragma once

#include "Upscaling/NvidiaDlss/DlssCapabilityReport.h"
#include "Upscaling/UpscalerProvider.h"

class NvidiaDlssUpscalerProvider final : public IUpscalerProvider
{
  public:
	EUpscalerProviderKind GetKind() const noexcept override { return EUpscalerProviderKind::NvidiaDlss; }
	std::string_view GetName() const noexcept override { return "NVIDIA DLSS"; }
	UpscalerProviderCapabilities QueryCapabilities(const RhiCapabilities& capabilities) const override;
	bool Initialize(const RhiCapabilities& capabilities) override;
	void SetupFrame(const UpscalerFrameSetupDesc& frameSetup) override;
	UpscalerEvaluationResult Evaluate(const UpscalerEvaluationDesc& evaluation) override;
	void OnResize(RenderViewportExtent renderExtent, RenderViewportExtent outputExtent) override;
	void ResetHistory(std::string_view reason) override;
	void Shutdown() noexcept override;
	UpscalerProviderCapabilities GetDiagnostics() const override { return m_diagnostics; }

  private:
	UpscalerProviderCapabilities m_diagnostics = {};
	DlssCapabilityReport m_dlssCapabilities = {};
	RenderViewportExtent m_renderExtent = {};
	RenderViewportExtent m_outputExtent = {};
};
