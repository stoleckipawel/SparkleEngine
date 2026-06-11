#include "../../PCH.h"
#include "Upscaling/NvidiaDlss/StreamlineDlssRuntime.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
#include <sl.h>
#include <sl_dlss.h>
#include <sl_helpers.h>
#endif

#include <array>
#include <filesystem>
#include <format>
#include <system_error>

namespace
{
	constexpr const char* kRuntimeNotIntegratedReason =
	    "NVIDIA Streamline SDK is not integrated in this build; DLSS Super Resolution remains unavailable.";

	bool HasNativeAdapterLuid(const RhiAdapterIdentity& adapter) noexcept
	{
		return adapter.NativeLuidSizeInBytes > 0 && adapter.NativeLuidSizeInBytes <= adapter.NativeLuid.size();
	}

	bool HasNativeEvaluationContract(const UpscalerEvaluationDesc& evaluation) noexcept
	{
		return evaluation.NativeCommandList && evaluation.NativeInputColor && evaluation.NativeDepth && evaluation.NativeMotionVectors &&
		       evaluation.NativeOutputColor;
	}

	DlssFeatureMatrix BuildUnavailableFeatureMatrix(std::string_view reason)
	{
		DlssFeatureMatrix matrix;
		matrix.Entries = {
		    DlssFeatureMatrixEntry{
		        .Feature = EDlssFeatureKind::SuperResolution,
		        .State = EDlssFeatureState::Unavailable,
		        .QualityModes = "NativeAA, Quality, Balanced, Performance, UltraPerformance",
		        .ModelPresetRecommendation = "SDK queried preset recommendation required.",
		        .RequiredResources = "HUD-less color, depth, motion vectors, exposure or auto-exposure, final output",
		        .Reason = std::string(reason)},
		    DlssFeatureMatrixEntry{
		        .Feature = EDlssFeatureKind::NativeAA,
		        .State = EDlssFeatureState::Unavailable,
		        .QualityModes = "NativeAA",
		        .ModelPresetRecommendation = "Reuse Super Resolution preset recommendation at render extent equal to output extent.",
		        .RequiredResources = "Same as Super Resolution; render extent must equal output extent",
		        .Reason = std::string(reason)},
		    DlssFeatureMatrixEntry{
		        .Feature = EDlssFeatureKind::RayReconstruction,
		        .State = EDlssFeatureState::Unavailable,
		        .QualityModes = "SDK queried modes required.",
		        .ModelPresetRecommendation = "SDK queried preset recommendation required.",
		        .RequiredResources = "Noisy indirect lighting signals plus guide buffers; not direct shadow visibility",
		        .Reason = std::string(reason)},
		    DlssFeatureMatrixEntry{
		        .Feature = EDlssFeatureKind::FrameGeneration,
		        .State = EDlssFeatureState::Unavailable,
		        .RequiresLatencyHook = true,
		        .QualityModes = "SDK queried generated-frame modes required.",
		        .ModelPresetRecommendation = "SDK queried frame-generation model recommendation required.",
		        .RequiredResources = "Present contract, frame IDs, optical-flow inputs, UI separation, latency hooks",
		        .Reason = std::string(reason)},
		    DlssFeatureMatrixEntry{
		        .Feature = EDlssFeatureKind::MultiFrameGeneration,
		        .State = EDlssFeatureState::Unavailable,
		        .RequiresLatencyHook = true,
		        .QualityModes = "SDK queried generated-frame multiplier modes required.",
		        .ModelPresetRecommendation = "SDK queried MFG model recommendation required.",
		        .RequiredResources = "Frame Generation contract plus SDK-reported MFG limits",
		        .Reason = std::string(reason)},
		    DlssFeatureMatrixEntry{
		        .Feature = EDlssFeatureKind::DynamicMultiFrameGeneration,
		        .State = EDlssFeatureState::Unavailable,
		        .RequiresLatencyHook = true,
		        .QualityModes = "SDK queried dynamic generated-frame modes required.",
		        .ModelPresetRecommendation = "SDK queried dynamic MFG model recommendation required.",
		        .RequiredResources = "MFG contract plus runtime frame-pacing and scheduling policy",
		        .Reason = std::string(reason)},
		    DlssFeatureMatrixEntry{
		        .Feature = EDlssFeatureKind::LatencyHook,
		        .State = EDlssFeatureState::Unavailable,
		        .QualityModes = "Required only by selected generated-frame paths.",
		        .ModelPresetRecommendation = "Not applicable.",
		        .RequiredResources = "Frame markers and latency hook points required by selected SDK feature",
		        .Reason = std::string(reason)}};
		return matrix;
	}

#if SPARKLE_WITH_NVIDIA_STREAMLINE
	constexpr const char* kStreamlineSdkVersion = "2.11.1";
	constexpr std::uint32_t kD3D12ResourceStateUnorderedAccess = 0x00000008u;
	constexpr std::uint32_t kD3D12ResourceStateDepthRead = 0x00000020u;
	constexpr std::uint32_t kD3D12ResourceStateNonPixelShaderResource = 0x00000040u;
	constexpr std::uint32_t kD3D12ResourceStatePixelShaderResource = 0x00000080u;
	constexpr std::uint32_t kD3D12ResourceStateCopySource = 0x00000800u;

	std::string FormatStreamlineFailure(std::string_view operation, sl::Result result)
	{
		return std::format("{} failed: {}", operation, sl::getResultAsStr(result));
	}

	sl::DLSSMode ToStreamlineDlssMode(EUpscalerQualityMode mode) noexcept
	{
		switch (mode)
		{
			case EUpscalerQualityMode::NativeAA:
				return sl::DLSSMode::eDLAA;
			case EUpscalerQualityMode::Quality:
				return sl::DLSSMode::eMaxQuality;
			case EUpscalerQualityMode::Balanced:
				return sl::DLSSMode::eBalanced;
			case EUpscalerQualityMode::Performance:
				return sl::DLSSMode::eMaxPerformance;
			case EUpscalerQualityMode::UltraPerformance:
				return sl::DLSSMode::eUltraPerformance;
		}

		return sl::DLSSMode::eMaxQuality;
	}

	DlssFeatureMatrix BuildStreamlineFeatureMatrix(bool superResolutionSupported, std::string_view reason)
	{
		DlssFeatureMatrix matrix = BuildUnavailableFeatureMatrix(reason);
		for (DlssFeatureMatrixEntry& entry : matrix.Entries)
		{
			if (entry.Feature == EDlssFeatureKind::SuperResolution || entry.Feature == EDlssFeatureKind::NativeAA)
			{
				entry.State = superResolutionSupported ? EDlssFeatureState::Available : EDlssFeatureState::Unavailable;
				entry.Supported = superResolutionSupported;
				entry.Reason = std::string(reason);
			}
		}
		return matrix;
	}

	void FillIdentity(sl::float4x4& matrix) noexcept
	{
		matrix = {};
		matrix[0] = sl::float4{1.0f, 0.0f, 0.0f, 0.0f};
		matrix[1] = sl::float4{0.0f, 1.0f, 0.0f, 0.0f};
		matrix[2] = sl::float4{0.0f, 0.0f, 1.0f, 0.0f};
		matrix[3] = sl::float4{0.0f, 0.0f, 0.0f, 1.0f};
	}

	void FillMinimalConstants(sl::Constants& constants, const UpscalerInputContract& inputContract) noexcept
	{
		FillIdentity(constants.cameraViewToClip);
		FillIdentity(constants.clipToCameraView);
		FillIdentity(constants.clipToLensClip);
		FillIdentity(constants.clipToPrevClip);
		FillIdentity(constants.prevClipToClip);
		constants.jitterOffset = sl::float2{inputContract.TemporalState.JitterCurrent.x, inputContract.TemporalState.JitterCurrent.y};
		constants.mvecScale = sl::float2{
		    inputContract.RenderExtent.Width > 0 ? 1.0f / static_cast<float>(inputContract.RenderExtent.Width) : 1.0f,
		    inputContract.RenderExtent.Height > 0 ? 1.0f / static_cast<float>(inputContract.RenderExtent.Height) : 1.0f};
		constants.cameraPinholeOffset = sl::float2{0.0f, 0.0f};
		constants.cameraPos = sl::float3{0.0f, 0.0f, 0.0f};
		constants.cameraUp = sl::float3{0.0f, 1.0f, 0.0f};
		constants.cameraRight = sl::float3{1.0f, 0.0f, 0.0f};
		constants.cameraFwd = sl::float3{0.0f, 0.0f, 1.0f};
		constants.cameraNear = 0.1f;
		constants.cameraFar = 100000.0f;
		constants.cameraFOV = 1.04719755f;
		constants.cameraAspectRatio =
		    inputContract.RenderExtent.Height > 0 ?
		        static_cast<float>(inputContract.RenderExtent.Width) / static_cast<float>(inputContract.RenderExtent.Height) :
		        1.0f;
		constants.depthInverted = sl::Boolean::eFalse;
		constants.cameraMotionIncluded = sl::Boolean::eTrue;
		constants.motionVectors3D = sl::Boolean::eFalse;
		constants.reset = inputContract.ResetRequested || !inputContract.TemporalState.HistoryValid ? sl::Boolean::eTrue : sl::Boolean::eFalse;
		constants.motionVectorsJittered = sl::Boolean::eTrue;
	}

	bool UpgradePresentationInterfaceWithStreamline(void** nativeInterface, void*) noexcept
	{
		return nativeInterface != nullptr && slUpgradeInterface(nativeInterface) == sl::Result::eOk;
	}

	class StreamlineDlssRuntime final : public IStreamlineDlssRuntime
	{
	  public:
		bool Initialize(const StreamlineDlssRuntimeDesc& desc) override
		{
			m_qualityMode = desc.QualityMode;
			m_diagnostics.SdkVersion = kStreamlineSdkVersion;
			m_diagnostics.SelectedQualityMode = UpscalerQualityModeToString(desc.QualityMode);

			if (desc.Capabilities.BackendApi != ERhiBackendApi::D3D12)
			{
				m_diagnostics.State = EDlssProviderRuntimeState::Unavailable;
				m_diagnostics.FailureReason = "Initial Streamline DLSS runtime is wired for D3D12; Vulkan support remains behind the same provider boundary.";
				m_diagnostics.FeatureMatrix = BuildStreamlineFeatureMatrix(false, m_diagnostics.FailureReason);
				return false;
			}
			if (!desc.NativeDevice)
			{
				m_diagnostics.State = EDlssProviderRuntimeState::Unavailable;
				m_diagnostics.FailureReason = "D3D12 native device handle is unavailable.";
				m_diagnostics.FeatureMatrix = BuildStreamlineFeatureMatrix(false, m_diagnostics.FailureReason);
				return false;
			}
			if (!HasNativeAdapterLuid(desc.Capabilities.ExternalFeatureInterop.Adapter))
			{
				m_diagnostics.State = EDlssProviderRuntimeState::Unavailable;
				m_diagnostics.FailureReason = "D3D12 adapter native LUID is unavailable for Streamline feature support query.";
				m_diagnostics.FeatureMatrix = BuildStreamlineFeatureMatrix(false, m_diagnostics.FailureReason);
				return false;
			}

			const sl::Feature features[] = {sl::kFeatureDLSS};
			sl::Preferences preferences{};
			preferences.showConsole = false;
			preferences.logLevel = desc.DiagnosticsEnabled ? sl::LogLevel::eVerbose : sl::LogLevel::eDefault;
			preferences.featuresToLoad = features;
			preferences.numFeaturesToLoad = static_cast<std::uint32_t>(std::size(features));
			preferences.flags = sl::PreferenceFlags::eDisableCLStateTracking | sl::PreferenceFlags::eUseManualHooking |
			                    sl::PreferenceFlags::eUseFrameBasedResourceTagging | sl::PreferenceFlags::eAllowOTA |
			                    sl::PreferenceFlags::eLoadDownloadedPlugins;
			preferences.applicationId = desc.ApplicationId;
			preferences.engine = sl::EngineType::eCustom;
			preferences.engineVersion = "SparkleEngine-Development";
			preferences.projectId = "535041524B4C45454E47494E45303031";
			preferences.renderAPI = sl::RenderAPI::eD3D12;
			std::error_code logPathError;
			m_streamlineLogPath =
			    (std::filesystem::current_path(logPathError) / ".." / ".." / "logs" / "Streamline").lexically_normal().wstring();
			if (!logPathError)
			{
				std::filesystem::create_directories(m_streamlineLogPath, logPathError);
				if (!logPathError)
				{
					preferences.pathToLogsAndData = m_streamlineLogPath.c_str();
				}
			}

			sl::Result result = slInit(preferences, sl::kSDKVersion);
			if (result != sl::Result::eOk)
			{
				m_diagnostics.State = EDlssProviderRuntimeState::Unavailable;
				m_diagnostics.FailureReason = FormatStreamlineFailure("slInit", result);
				m_diagnostics.FeatureMatrix = BuildStreamlineFeatureMatrix(false, m_diagnostics.FailureReason);
				return false;
			}
			m_initialized = true;

			if (desc.PresentationBridge &&
			    !desc.PresentationBridge.UpgradePresentationInterface(
			        &UpgradePresentationInterfaceWithStreamline,
			        nullptr,
			        desc.PresentationBridge.UserData))
			{
				m_diagnostics.State = EDlssProviderRuntimeState::Unavailable;
				m_diagnostics.FailureReason = "Streamline failed to upgrade the RHI presentation interface for manual present hooks.";
				m_diagnostics.FeatureMatrix = BuildStreamlineFeatureMatrix(false, m_diagnostics.FailureReason);
				return false;
			}

			result = slSetD3DDevice(desc.NativeDevice.Value);
			if (result != sl::Result::eOk)
			{
				m_diagnostics.State = EDlssProviderRuntimeState::Unavailable;
				m_diagnostics.FailureReason = FormatStreamlineFailure("slSetD3DDevice", result);
				m_diagnostics.FeatureMatrix = BuildStreamlineFeatureMatrix(false, m_diagnostics.FailureReason);
				return false;
			}

			std::array<std::uint8_t, 8> adapterLuid = desc.Capabilities.ExternalFeatureInterop.Adapter.NativeLuid;
			sl::AdapterInfo adapterInfo{};
			adapterInfo.deviceLUID = adapterLuid.data();
			adapterInfo.deviceLUIDSizeInBytes = desc.Capabilities.ExternalFeatureInterop.Adapter.NativeLuidSizeInBytes;
			result = slIsFeatureSupported(sl::kFeatureDLSS, adapterInfo);
			if (result != sl::Result::eOk)
			{
				m_diagnostics.State = EDlssProviderRuntimeState::Unavailable;
				m_diagnostics.FailureReason = FormatStreamlineFailure("slIsFeatureSupported(DLSS)", result);
				m_diagnostics.FeatureMatrix = BuildStreamlineFeatureMatrix(false, m_diagnostics.FailureReason);
				return false;
			}

			m_viewport = sl::ViewportHandle{0u};
			m_diagnostics.State = EDlssProviderRuntimeState::Created;
			m_diagnostics.FeatureMatrix = BuildStreamlineFeatureMatrix(true, "DLSS Super Resolution is supported by Streamline.");
			return true;
		}

		bool SetupFrame(const UpscalerInputContract& inputContract) override
		{
			m_lastFrameContract = inputContract;
			m_diagnostics.RenderExtent = inputContract.RenderExtent;
			m_diagnostics.OutputExtent = inputContract.OutputExtent;
			m_diagnostics.ResetRequested = inputContract.ResetRequested;
			m_diagnostics.ResetReason = inputContract.ResetReason;
			return m_initialized;
		}

		UpscalerEvaluationResult Evaluate(const UpscalerEvaluationDesc& evaluation) override
		{
			if (!m_initialized)
			{
				m_diagnostics.State = EDlssProviderRuntimeState::FailedWithFallback;
				return {.ProducedOutput = false, .UsedFallback = true, .Reason = "Streamline DLSS runtime is not initialized."};
			}
			if (!HasNativeEvaluationContract(evaluation))
			{
				m_diagnostics.State = EDlssProviderRuntimeState::FailedWithFallback;
				m_diagnostics.FailureReason = "DLSS evaluation contract is missing a native command list or required native resources.";
				return {.ProducedOutput = false, .UsedFallback = true, .Reason = m_diagnostics.FailureReason};
			}

			const std::uint32_t frameIndex = static_cast<std::uint32_t>(m_lastFrameContract.FrameIndex);
			sl::FrameToken* frameToken = nullptr;
			sl::Result result = slGetNewFrameToken(frameToken, &frameIndex);
			if (result != sl::Result::eOk || frameToken == nullptr)
			{
				m_diagnostics.State = EDlssProviderRuntimeState::FailedWithFallback;
				m_diagnostics.FailureReason = FormatStreamlineFailure("slGetNewFrameToken", result);
				return {.ProducedOutput = false, .UsedFallback = true, .Reason = m_diagnostics.FailureReason};
			}

			sl::DLSSOptions options{};
			options.mode = ToStreamlineDlssMode(m_qualityMode);
			options.outputWidth = evaluation.OutputExtent.Width;
			options.outputHeight = evaluation.OutputExtent.Height;
			options.colorBuffersHDR = sl::Boolean::eTrue;
			options.useAutoExposure = sl::Boolean::eTrue;
			options.alphaUpscalingEnabled = sl::Boolean::eFalse;
			options.dlaaPreset = sl::DLSSPreset::ePresetK;
			options.qualityPreset = sl::DLSSPreset::ePresetK;
			options.balancedPreset = sl::DLSSPreset::ePresetK;
			options.performancePreset = sl::DLSSPreset::ePresetM;
			options.ultraPerformancePreset = sl::DLSSPreset::ePresetL;
			result = slDLSSSetOptions(m_viewport, options);
			if (result != sl::Result::eOk)
			{
				m_diagnostics.State = EDlssProviderRuntimeState::FailedWithFallback;
				m_diagnostics.FailureReason = FormatStreamlineFailure("slDLSSSetOptions", result);
				return {.ProducedOutput = false, .UsedFallback = true, .Reason = m_diagnostics.FailureReason};
			}

			sl::Constants constants{};
			FillMinimalConstants(constants, m_lastFrameContract);
			result = slSetConstants(constants, *frameToken, m_viewport);
			if (result != sl::Result::eOk)
			{
				m_diagnostics.State = EDlssProviderRuntimeState::FailedWithFallback;
				m_diagnostics.FailureReason = FormatStreamlineFailure("slSetConstants", result);
				return {.ProducedOutput = false, .UsedFallback = true, .Reason = m_diagnostics.FailureReason};
			}

			sl::Extent renderExtent{.top = 0, .left = 0, .width = evaluation.RenderExtent.Width, .height = evaluation.RenderExtent.Height};
			sl::Extent outputExtent{.top = 0, .left = 0, .width = evaluation.OutputExtent.Width, .height = evaluation.OutputExtent.Height};
			sl::Resource colorIn{sl::ResourceType::eTex2d, evaluation.NativeInputColor.Value, kD3D12ResourceStateCopySource};
			sl::Resource colorOut{sl::ResourceType::eTex2d, evaluation.NativeOutputColor.Value, kD3D12ResourceStateUnorderedAccess};
			sl::Resource depth{sl::ResourceType::eTex2d, evaluation.NativeDepth.Value, kD3D12ResourceStateDepthRead};
			sl::Resource motionVectors{
			    sl::ResourceType::eTex2d,
			    evaluation.NativeMotionVectors.Value,
			    kD3D12ResourceStateNonPixelShaderResource | kD3D12ResourceStatePixelShaderResource};
			std::array<sl::ResourceTag, 4> tags = {
			    sl::ResourceTag{&colorIn, sl::kBufferTypeScalingInputColor, sl::ResourceLifecycle::eOnlyValidNow, &renderExtent},
			    sl::ResourceTag{&colorOut, sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eOnlyValidNow, &outputExtent},
			    sl::ResourceTag{&depth, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilEvaluate, &renderExtent},
			    sl::ResourceTag{&motionVectors, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eOnlyValidNow, &renderExtent}};

			auto* commandBuffer = static_cast<sl::CommandBuffer*>(evaluation.NativeCommandList.Value);
			result = slSetTagForFrame(*frameToken, m_viewport, tags.data(), static_cast<std::uint32_t>(tags.size()), commandBuffer);
			if (result != sl::Result::eOk)
			{
				m_diagnostics.State = EDlssProviderRuntimeState::FailedWithFallback;
				m_diagnostics.FailureReason = FormatStreamlineFailure("slSetTagForFrame", result);
				return {.ProducedOutput = false, .UsedFallback = true, .Reason = m_diagnostics.FailureReason};
			}

			const sl::BaseStructure* inputs[] = {&m_viewport};
			result = slEvaluateFeature(sl::kFeatureDLSS, *frameToken, inputs, static_cast<std::uint32_t>(std::size(inputs)), commandBuffer);
			if (result != sl::Result::eOk)
			{
				m_diagnostics.State = EDlssProviderRuntimeState::FailedWithFallback;
				m_diagnostics.FailureReason = FormatStreamlineFailure("slEvaluateFeature(DLSS)", result);
				return {.ProducedOutput = false, .UsedFallback = true, .Reason = m_diagnostics.FailureReason};
			}

			m_diagnostics.State = EDlssProviderRuntimeState::Evaluating;
			m_diagnostics.FailureReason.clear();
			return {.ProducedOutput = true, .UsedFallback = false, .Reason = "Streamline DLSS evaluated successfully."};
		}

		void ResetHistory(std::string_view reason) override
		{
			m_diagnostics.ResetRequested = true;
			m_diagnostics.ResetReason = std::string(reason);
		}

		void Shutdown() noexcept override
		{
			if (m_initialized)
			{
				(void) slShutdown();
				m_initialized = false;
			}
			m_diagnostics.State = EDlssProviderRuntimeState::NotSelected;
		}

		const StreamlineDlssRuntimeDiagnostics& GetDiagnostics() const noexcept override { return m_diagnostics; }

	  private:
		StreamlineDlssRuntimeDiagnostics m_diagnostics = {};
		UpscalerInputContract m_lastFrameContract = {};
		std::wstring m_streamlineLogPath;
		sl::ViewportHandle m_viewport{0u};
		EUpscalerQualityMode m_qualityMode = EUpscalerQualityMode::Quality;
		bool m_initialized = false;
	};
#endif

	class UnavailableStreamlineDlssRuntime final : public IStreamlineDlssRuntime
	{
	  public:
		bool Initialize(const StreamlineDlssRuntimeDesc& desc) override
		{
			m_diagnostics.State = EDlssProviderRuntimeState::Unavailable;
			m_diagnostics.SdkVersion = "not-integrated";
			m_diagnostics.SelectedQualityMode = UpscalerQualityModeToString(desc.QualityMode);
			m_diagnostics.FeatureMatrix = BuildUnavailableFeatureMatrix(kRuntimeNotIntegratedReason);
			m_diagnostics.FailureReason = kRuntimeNotIntegratedReason;
			return false;
		}

		bool SetupFrame(const UpscalerInputContract& inputContract) override
		{
			m_diagnostics.RenderExtent = inputContract.RenderExtent;
			m_diagnostics.OutputExtent = inputContract.OutputExtent;
			m_diagnostics.ResetRequested = inputContract.ResetRequested;
			m_diagnostics.ResetReason = inputContract.ResetReason;
			m_lastFrameContract = inputContract;
			return false;
		}

		UpscalerEvaluationResult Evaluate(const UpscalerEvaluationDesc& evaluation) override
		{
			m_diagnostics.State = EDlssProviderRuntimeState::FailedWithFallback;
			m_diagnostics.RenderExtent = evaluation.RenderExtent;
			m_diagnostics.OutputExtent = evaluation.OutputExtent;
			m_diagnostics.FailureReason =
			    HasNativeEvaluationContract(evaluation)
			        ? kRuntimeNotIntegratedReason
			        : "DLSS evaluation contract is missing a native command list or required native resources.";
			return UpscalerEvaluationResult{
			    .ProducedOutput = false,
			    .UsedFallback = true,
			    .Reason = m_diagnostics.FailureReason};
		}

		void ResetHistory(std::string_view reason) override
		{
			m_diagnostics.ResetRequested = true;
			m_diagnostics.ResetReason = std::string(reason);
		}

		void Shutdown() noexcept override
		{
			m_diagnostics.State = EDlssProviderRuntimeState::NotSelected;
		}

		const StreamlineDlssRuntimeDiagnostics& GetDiagnostics() const noexcept override { return m_diagnostics; }

	  private:
		StreamlineDlssRuntimeDiagnostics m_diagnostics = {};
		UpscalerInputContract m_lastFrameContract = {};
	};
}

const char* DlssProviderRuntimeStateToString(EDlssProviderRuntimeState state) noexcept
{
	switch (state)
	{
		case EDlssProviderRuntimeState::NotSelected:
			return "NotSelected";
		case EDlssProviderRuntimeState::Unavailable:
			return "Unavailable";
		case EDlssProviderRuntimeState::AvailableNotCreated:
			return "AvailableNotCreated";
		case EDlssProviderRuntimeState::Created:
			return "Created";
		case EDlssProviderRuntimeState::Evaluating:
			return "Evaluating";
		case EDlssProviderRuntimeState::FailedWithFallback:
			return "FailedWithFallback";
	}

	return "Unknown";
}

const char* DlssFeatureKindToString(EDlssFeatureKind feature) noexcept
{
	switch (feature)
	{
		case EDlssFeatureKind::SuperResolution:
			return "SuperResolution";
		case EDlssFeatureKind::NativeAA:
			return "NativeAA";
		case EDlssFeatureKind::RayReconstruction:
			return "RayReconstruction";
		case EDlssFeatureKind::FrameGeneration:
			return "FrameGeneration";
		case EDlssFeatureKind::MultiFrameGeneration:
			return "MultiFrameGeneration";
		case EDlssFeatureKind::DynamicMultiFrameGeneration:
			return "DynamicMultiFrameGeneration";
		case EDlssFeatureKind::LatencyHook:
			return "LatencyHook";
	}

	return "Unknown";
}

const char* DlssFeatureStateToString(EDlssFeatureState state) noexcept
{
	switch (state)
	{
		case EDlssFeatureState::NotSelected:
			return "NotSelected";
		case EDlssFeatureState::Unavailable:
			return "Unavailable";
		case EDlssFeatureState::Available:
			return "Available";
		case EDlssFeatureState::Enabled:
			return "Enabled";
		case EDlssFeatureState::Active:
			return "Active";
		case EDlssFeatureState::FailedWithFallback:
			return "FailedWithFallback";
	}

	return "Unknown";
}

StreamlineDlssRuntimeCapabilities QueryStreamlineDlssRuntimeCapabilities(const RhiCapabilities& capabilities) noexcept
{
#if SPARKLE_WITH_NVIDIA_STREAMLINE
	const bool bridgeReady = capabilities.BackendApi == ERhiBackendApi::D3D12 &&
	                         capabilities.ExternalFeatureInterop.ExposesNativeDevice &&
	                         capabilities.ExternalFeatureInterop.ExposesNativeGraphicsCommandList &&
	                         capabilities.ExternalFeatureInterop.ExposesNativeResources;
	const std::string reason = bridgeReady ? "NVIDIA Streamline SDK is integrated; runtime support will be verified during provider initialization."
	                                      : "D3D12 native device, command-list, or resource interop is unavailable.";
	return StreamlineDlssRuntimeCapabilities{
	    .RuntimeIntegrated = true,
	    .RuntimeAvailable = bridgeReady,
	    .FeatureQuerySucceeded = bridgeReady,
	    .FeatureSupported = bridgeReady,
	    .FeatureMatrix = BuildStreamlineFeatureMatrix(bridgeReady, reason),
	    .SdkVersion = kStreamlineSdkVersion,
	    .Reason = reason};
#else
	return StreamlineDlssRuntimeCapabilities{
	    .RuntimeIntegrated = false,
	    .RuntimeAvailable = false,
	    .FeatureQuerySucceeded = false,
	    .FeatureSupported = false,
	    .FeatureMatrix = BuildUnavailableFeatureMatrix(kRuntimeNotIntegratedReason),
	    .SdkVersion = "not-integrated",
	    .Reason = kRuntimeNotIntegratedReason};
#endif
}

std::unique_ptr<IStreamlineDlssRuntime> CreateStreamlineDlssRuntime()
{
#if SPARKLE_WITH_NVIDIA_STREAMLINE
	return std::make_unique<StreamlineDlssRuntime>();
#else
	return std::make_unique<UnavailableStreamlineDlssRuntime>();
#endif
}
