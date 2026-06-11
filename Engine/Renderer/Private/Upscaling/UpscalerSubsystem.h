#pragma once

#include "Upscaling/UpscalerProvider.h"
#include "Upscaling/UpscalerSettings.h"

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

	void Initialize(const RhiCapabilities& capabilities);
	void SetupFrame(const UpscalerFrameSetupDesc& frameSetup);
	UpscalerEvaluationResult Evaluate(const UpscalerEvaluationDesc& evaluation);
	void OnResize(RenderViewportExtent renderExtent, RenderViewportExtent outputExtent);
	void ResetHistory(std::string_view reason);
	void Shutdown() noexcept;

	const IUpscalerProvider& GetActiveProvider() const noexcept { return *m_activeProvider; }
	const UpscalerProviderCapabilities& GetDiagnostics() const noexcept { return m_diagnostics; }
	EUpscalerProviderKind GetRequestedProviderKind() const noexcept { return m_settings.RequestedProvider; }

  private:
	static std::unique_ptr<IUpscalerProvider> CreateProvider(EUpscalerProviderKind kind);
	static std::unique_ptr<IUpscalerProvider> CreateFallbackProvider();

	UpscalerSettings m_settings = {};
	std::unique_ptr<IUpscalerProvider> m_activeProvider;
	UpscalerProviderCapabilities m_diagnostics = {};
	bool m_shutdown = true;
};
