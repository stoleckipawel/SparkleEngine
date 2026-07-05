#pragma once

#include "Upscaling/UpscalerInputContract.h"
#include "Upscaling/UpscalerProvider.h"
#include "Upscaling/UpscalerSettings.h"
#include "RHI/Public/Core/RhiCapabilities.h"

#include <memory>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

enum class EDlssProviderRuntimeState : std::uint8_t
{
	NotSelected = 0,
	Unavailable = 1,
	AvailableNotCreated = 2,
	Created = 3,
	Evaluating = 4,
	Failed = 5
};

enum class EDlssFeatureKind : std::uint8_t
{
	SuperResolution = 0,
	NativeAA = 1
};

enum class EDlssFeatureState : std::uint8_t
{
	NotSelected = 0,
	Unavailable = 1,
	Available = 2,
	Enabled = 3,
	Active = 4,
	Failed = 5
};

struct DlssFeatureMatrixEntry final
{
	EDlssFeatureKind Feature = EDlssFeatureKind::SuperResolution;
	EDlssFeatureState State = EDlssFeatureState::Unavailable;
	bool Supported = false;
	std::string QualityModes;
	std::string ModelPresetRecommendation;
	std::string RequiredResources;
	std::string Reason;
};

struct DlssFeatureMatrix final
{
	std::vector<DlssFeatureMatrixEntry> Entries;
};

struct StreamlineDlssRuntimeCapabilities final
{
	bool RuntimeIntegrated = false;
	bool RuntimeAvailable = false;
	bool FeatureQuerySucceeded = false;
	bool FeatureSupported = false;
	EUpscalerProviderFailureDomain FailureDomain = EUpscalerProviderFailureDomain::None;
	DlssFeatureMatrix FeatureMatrix;
	std::string SdkVersion;
	std::string Reason;
};

struct StreamlineDlssRuntimeDesc final
{
	RhiCapabilities Capabilities = {};
	RhiNativeDeviceQueueInterop NativeInterop = {};
	UpscalerPresentationBridge PresentationBridge = {};
	EUpscalerQualityMode QualityMode = EUpscalerQualityMode::Quality;
};

struct StreamlineDlssRuntimeDiagnostics final
{
	EDlssProviderRuntimeState State = EDlssProviderRuntimeState::NotSelected;
	std::string SdkVersion;
	std::string SelectedQualityMode;
	DlssFeatureMatrix FeatureMatrix;
	RenderViewportExtent RenderExtent = {};
	RenderViewportExtent OutputExtent = {};
	bool ResetRequested = false;
	std::string ResetReason;
	EUpscalerProviderFailureDomain FailureDomain = EUpscalerProviderFailureDomain::None;
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
const char* DlssFeatureKindToString(EDlssFeatureKind feature) noexcept;
const char* DlssFeatureStateToString(EDlssFeatureState state) noexcept;
StreamlineDlssRuntimeCapabilities QueryStreamlineDlssRuntimeCapabilities(const RhiCapabilities& capabilities) noexcept;
std::unique_ptr<IStreamlineDlssRuntime> CreateStreamlineDlssRuntime();
