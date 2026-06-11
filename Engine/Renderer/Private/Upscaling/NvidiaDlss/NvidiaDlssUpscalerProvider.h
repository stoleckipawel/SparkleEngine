#pragma once

#include "Upscaling/NvidiaDlss/DlssCapabilityReport.h"
#include "Upscaling/NvidiaDlss/StreamlineDlssRuntime.h"
#include "Upscaling/UpscalerProvider.h"
#include "Upscaling/UpscalerSettings.h"

#include <memory>

class NvidiaDlssUpscalerProvider final : public IUpscalerProvider
{
  public:
	EUpscalerProviderKind GetKind() const noexcept override { return EUpscalerProviderKind::NvidiaDlss; }
	std::string_view GetName() const noexcept override { return "NVIDIA DLSS"; }
	UpscalerProviderCapabilities QueryCapabilities(const RhiCapabilities& capabilities) const override;
	bool Initialize(const RhiCapabilities& capabilities, NativeGraphicsDeviceHandle nativeDevice) override;
	void SetupFrame(const UpscalerInputContract& inputContract) override;
	UpscalerEvaluationResult Evaluate(const UpscalerEvaluationDesc& evaluation) override;
	void OnResize(RenderViewportExtent renderExtent, RenderViewportExtent outputExtent) override;
	void ResetHistory(std::string_view reason) override;
	void Shutdown() noexcept override;
	UpscalerProviderCapabilities GetDiagnostics() const override;

  private:
	UpscalerProviderCapabilities m_diagnostics = {};
	DlssCapabilityReport m_dlssCapabilities = {};
	UpscalerInputContract m_lastInputContract = {};
	std::unique_ptr<IStreamlineDlssRuntime> m_runtime;
	EUpscalerQualityMode m_qualityMode = EUpscalerQualityMode::Quality;
	RenderViewportExtent m_renderExtent = {};
	RenderViewportExtent m_outputExtent = {};
};
