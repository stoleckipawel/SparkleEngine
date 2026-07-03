#pragma once

#include "RayReconstruction/RayReconstructionInputContract.h"
#include "RayReconstruction/RayReconstructionProvider.h"
#include "RayReconstruction/RayReconstructionSettings.h"
#include "RHI/Public/Interop/RhiInteropService.h"

#include <memory>

struct RhiCapabilities;

class RayReconstructionSubsystem final
{
  public:
	RayReconstructionSubsystem() = default;
	~RayReconstructionSubsystem() noexcept;

	RayReconstructionSubsystem(const RayReconstructionSubsystem&) = delete;
	RayReconstructionSubsystem& operator=(const RayReconstructionSubsystem&) = delete;
	RayReconstructionSubsystem(RayReconstructionSubsystem&&) = delete;
	RayReconstructionSubsystem& operator=(RayReconstructionSubsystem&&) = delete;

	void Initialize(
	    const RhiCapabilities& capabilities,
	    RhiNativeDeviceQueueInterop nativeInterop,
	    RayReconstructionPresentationBridge presentationBridge);
	void SetupFrame(const RayReconstructionInputContract& inputContract);
	RayReconstructionEvaluationResult Evaluate(const RayReconstructionEvaluationDesc& evaluation);
	void OnResize(RenderViewportExtent renderExtent, RenderViewportExtent outputExtent);
	void ResetHistory(std::string_view reason);
	void Shutdown() noexcept;

	const RayReconstructionProviderCapabilities& GetDiagnostics() const noexcept { return m_diagnostics; }
	EngineRayReconstructionMode GetRequestedMode() const noexcept { return m_settings.Mode; }

  private:
	static std::unique_ptr<IRayReconstructionProvider> CreateProvider(EngineRayReconstructionMode mode);
	void RefreshDiagnostics(IRayReconstructionProvider* provider) noexcept;

	RayReconstructionSettings m_settings = {};
	std::unique_ptr<IRayReconstructionProvider> m_activeProvider;
	RayReconstructionProviderCapabilities m_diagnostics = {};
	RayReconstructionInputContractValidation m_lastInputValidation = {};
	RhiNativeDeviceQueueInterop m_nativeInterop = {};
	RayReconstructionPresentationBridge m_presentationBridge = {};
	bool m_shutdown = true;
};
