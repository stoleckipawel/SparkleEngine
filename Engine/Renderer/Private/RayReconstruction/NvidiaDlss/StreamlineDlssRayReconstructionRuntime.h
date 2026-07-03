#pragma once

#include "RayReconstruction/RayReconstructionInputContract.h"
#include "RayReconstruction/RayReconstructionProvider.h"
#include "RayReconstruction/RayReconstructionSettings.h"
#include "RHI/Public/Core/RhiCapabilities.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

enum class EDlssRayReconstructionRuntimeState : std::uint8_t
{
	NotSelected = 0,
	Unavailable = 1,
	Created = 2,
	Evaluating = 3,
	FailedWithFallback = 4,
};

struct StreamlineDlssRayReconstructionRuntimeDesc final
{
	RhiCapabilities Capabilities = {};
	RhiNativeDeviceQueueInterop NativeInterop = {};
	RayReconstructionPresentationBridge PresentationBridge = {};
	EngineRayReconstructionQualityMode QualityMode = EngineRayReconstructionQualityMode::Quality;
};

struct StreamlineDlssRayReconstructionRuntimeDiagnostics final
{
	EDlssRayReconstructionRuntimeState State = EDlssRayReconstructionRuntimeState::NotSelected;
	std::string SdkVersion;
	std::string SelectedQualityMode;
	RenderViewportExtent RenderExtent = {};
	RenderViewportExtent OutputExtent = {};
	bool ResetRequested = false;
	std::string ResetReason;
	ERayReconstructionProviderFailureDomain FailureDomain = ERayReconstructionProviderFailureDomain::None;
	std::string FailureReason;
};

class IStreamlineDlssRayReconstructionRuntime
{
  public:
	virtual ~IStreamlineDlssRayReconstructionRuntime() = default;

	virtual bool Initialize(const StreamlineDlssRayReconstructionRuntimeDesc& desc) = 0;
	virtual bool SetupFrame(const RayReconstructionInputContract& inputContract) = 0;
	virtual RayReconstructionEvaluationResult Evaluate(const RayReconstructionEvaluationDesc& evaluation) = 0;
	virtual void ResetHistory(std::string_view reason) = 0;
	virtual void Shutdown() noexcept = 0;
	virtual const StreamlineDlssRayReconstructionRuntimeDiagnostics& GetDiagnostics() const noexcept = 0;
};

const char* DlssRayReconstructionRuntimeStateToString(EDlssRayReconstructionRuntimeState state) noexcept;
std::unique_ptr<IStreamlineDlssRayReconstructionRuntime> CreateStreamlineDlssRayReconstructionRuntime();
