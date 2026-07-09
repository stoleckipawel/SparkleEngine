#pragma once

#include "Upscaling/UpscalerProvider.h"
#include "RHI/Public/Interop/RhiInteropService.h"

#include <memory>

struct RhiCapabilities;

// Renderer-level orchestrator for upscaler providers. It selects and owns the
// active provider, while FrameGraph/presentation integration remains provider
// neutral and RHI remains limited to backend capability facts.
class UpscalerSubsystem final
{
  public:
	UpscalerSubsystem() = default;
	~UpscalerSubsystem() noexcept;

	UpscalerSubsystem(const UpscalerSubsystem&) = delete;
	UpscalerSubsystem& operator=(const UpscalerSubsystem&) = delete;
	UpscalerSubsystem(UpscalerSubsystem&&) = delete;
	UpscalerSubsystem& operator=(UpscalerSubsystem&&) = delete;

	void Initialize(
	    const RhiCapabilities& capabilities,
	    RhiNativeDeviceQueueInterop nativeInterop,
	    UpscalerPresentationBridge presentationBridge);
	void SetupFrame(const UpscalerInputContract& inputContract);
	UpscalerEvaluationResult Evaluate(const UpscalerEvaluationDesc& evaluation);
	void OnResize(RenderViewportExtent renderExtent, RenderViewportExtent outputExtent);
	void ResetHistory(std::string_view reason);
	void Shutdown() noexcept;

	const UpscalerProviderCapabilities& GetDiagnostics() const noexcept { return m_diagnostics; }

  private:
	static std::unique_ptr<IUpscalerProvider> CreateProvider(EUpscalerProviderKind kind);
	void RefreshDiagnostics(IUpscalerProvider* provider) noexcept;

	std::unique_ptr<IUpscalerProvider> m_activeProvider;
	UpscalerProviderCapabilities m_diagnostics = {};
	UpscalerInputContractValidation m_lastInputValidation = {};
	RhiNativeDeviceQueueInterop m_nativeInterop = {};
	UpscalerPresentationBridge m_presentationBridge = {};
	bool m_shutdown = true;
};
