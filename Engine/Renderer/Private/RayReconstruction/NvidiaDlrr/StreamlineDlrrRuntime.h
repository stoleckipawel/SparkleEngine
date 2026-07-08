#pragma once

#include "RayReconstruction/RayReconstructionInputContract.h"
#include "RayReconstruction/RayReconstructionProvider.h"
#include "RayReconstruction/RayReconstructionSettings.h"
#include "RHI/Public/Core/RhiCapabilities.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

enum class EDlrrRuntimeState : std::uint8_t
{
	NotSelected = 0,
	Unavailable = 1,
	Created = 2,
	Evaluating = 3,
	Failed = 4,
};

struct StreamlineDlrrRuntimeDesc final
{
	RhiCapabilities Capabilities = {};
	RhiNativeDeviceQueueInterop NativeInterop = {};
	RayReconstructionPresentationBridge PresentationBridge = {};
};

struct StreamlineDlrrRuntimeDiagnostics final
{
	EDlrrRuntimeState State = EDlrrRuntimeState::NotSelected;
	ERayReconstructionProviderFailureDomain FailureDomain = ERayReconstructionProviderFailureDomain::None;
	std::string FailureReason;
};

class IStreamlineDlrrRuntime
{
  public:
	virtual ~IStreamlineDlrrRuntime() = default;

	virtual bool Initialize(const StreamlineDlrrRuntimeDesc& desc) = 0;
	virtual bool SetupFrame(const RayReconstructionInputContract& inputContract) = 0;
	virtual RayReconstructionEvaluationResult Evaluate(const RayReconstructionEvaluationDesc& evaluation) = 0;
	virtual void ResetHistory(std::string_view reason) = 0;
	virtual void Shutdown() noexcept = 0;
	virtual const StreamlineDlrrRuntimeDiagnostics& GetDiagnostics() const noexcept = 0;
};

std::unique_ptr<IStreamlineDlrrRuntime> CreateStreamlineDlrrRuntime();
