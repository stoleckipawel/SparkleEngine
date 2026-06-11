#pragma once

#include "Upscaling/UpscalerInputContract.h"
#include "Upscaling/UpscalerProvider.h"
#include "Upscaling/UpscalerSettings.h"
#include "RHI/Public/Core/RhiCapabilities.h"

#include <memory>
#include <cstdint>
#include <string>
#include <string_view>

enum class EDlssProviderRuntimeState : std::uint8_t
{
	NotSelected = 0,
	Unavailable = 1,
	AvailableNotCreated = 2,
	Created = 3,
	Evaluating = 4,
	FailedWithFallback = 5
};

struct StreamlineDlssRuntimeCapabilities final
{
	bool RuntimeIntegrated = false;
	bool RuntimeAvailable = false;
	bool FeatureQuerySucceeded = false;
	bool FeatureSupported = false;
	std::string SdkVersion;
	std::string Reason;
};

struct StreamlineDlssRuntimeDesc final
{
	RhiCapabilities Capabilities = {};
	EUpscalerQualityMode QualityMode = EUpscalerQualityMode::Quality;
	std::string ApplicationName;
	std::uint32_t ApplicationId = 0;
};

struct StreamlineDlssRuntimeDiagnostics final
{
	EDlssProviderRuntimeState State = EDlssProviderRuntimeState::NotSelected;
	std::string SdkVersion;
	std::string SelectedQualityMode;
	RenderViewportExtent RenderExtent = {};
	RenderViewportExtent OutputExtent = {};
	bool ResetRequested = false;
	std::string ResetReason;
	std::string FailureReason;
};

class IStreamlineDlssRuntime
{
  public:
	virtual ~IStreamlineDlssRuntime() = default;

	virtual bool Initialize(const StreamlineDlssRuntimeDesc& desc) = 0;
	virtual bool SetupFrame(const UpscalerInputContract& inputContract) = 0;
	virtual UpscalerEvaluationResult Evaluate(const UpscalerEvaluationDesc& evaluation) = 0;
	virtual void ResetHistory(std::string_view reason) = 0;
	virtual void Shutdown() noexcept = 0;
	virtual const StreamlineDlssRuntimeDiagnostics& GetDiagnostics() const noexcept = 0;
};

const char* DlssProviderRuntimeStateToString(EDlssProviderRuntimeState state) noexcept;
StreamlineDlssRuntimeCapabilities QueryStreamlineDlssRuntimeCapabilities(const RhiCapabilities& capabilities) noexcept;
std::unique_ptr<IStreamlineDlssRuntime> CreateStreamlineDlssRuntime();
