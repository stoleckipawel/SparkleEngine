#pragma once

#include "Upscaling/UpscalerProvider.h"

class PassthroughUpscalerProvider final : public IUpscalerProvider
{
  public:
	EUpscalerProviderKind GetKind() const noexcept override { return EUpscalerProviderKind::Passthrough; }
	std::string_view GetName() const noexcept override { return "Passthrough"; }
	UpscalerProviderCapabilities QueryCapabilities(const RhiCapabilities& capabilities) const override;
	bool Initialize(
	    const RhiCapabilities& capabilities,
	    NativeGraphicsDeviceHandle nativeDevice,
	    UpscalerPresentationBridge presentationBridge) override;
	void SetupFrame(const UpscalerInputContract& inputContract) override;
	UpscalerEvaluationResult Evaluate(const UpscalerEvaluationDesc& evaluation) override;
	void OnResize(RenderViewportExtent renderExtent, RenderViewportExtent outputExtent) override;
	void ResetHistory(std::string_view reason) override;
	void Shutdown() noexcept override;
	UpscalerProviderCapabilities GetDiagnostics() const override { return m_diagnostics; }

  private:
	UpscalerProviderCapabilities m_diagnostics = {};
	RenderViewportExtent m_renderExtent = {};
	RenderViewportExtent m_outputExtent = {};
};
