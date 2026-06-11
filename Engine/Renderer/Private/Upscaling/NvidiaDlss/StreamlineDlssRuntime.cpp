#include "../../PCH.h"
#include "Upscaling/NvidiaDlss/StreamlineDlssRuntime.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
#include <sl.h>
#include <sl_dlss.h>
#include <sl_helpers.h>
#endif

#include <array>
#include <cmath>
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

	bool ExtentsMatch(RenderViewportExtent lhs, RenderViewportExtent rhs) noexcept
	{
		return lhs.Width == rhs.Width && lhs.Height == rhs.Height;
	}

	EUpscalerQualityMode ResolveFrameQualityMode(
	    EUpscalerQualityMode requestedMode,
	    RenderViewportExtent renderExtent,
	    RenderViewportExtent outputExtent) noexcept
	{
		if (requestedMode != EUpscalerQualityMode::NativeAA && ExtentsMatch(renderExtent, outputExtent))
		{
			return EUpscalerQualityMode::NativeAA;
		}
		return requestedMode;
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

	sl::float4x4 ToStreamlineMatrix(const DirectX::XMFLOAT4X4& source) noexcept
	{
		sl::float4x4 matrix{};
		matrix[0] = sl::float4{source.m[0][0], source.m[0][1], source.m[0][2], source.m[0][3]};
		matrix[1] = sl::float4{source.m[1][0], source.m[1][1], source.m[1][2], source.m[1][3]};
		matrix[2] = sl::float4{source.m[2][0], source.m[2][1], source.m[2][2], source.m[2][3]};
		matrix[3] = sl::float4{source.m[3][0], source.m[3][1], source.m[3][2], source.m[3][3]};
		return matrix;
	}

	sl::float4x4 ToStreamlineMatrix(DirectX::FXMMATRIX source) noexcept
	{
		DirectX::XMFLOAT4X4 stored{};
		DirectX::XMStoreFloat4x4(&stored, source);
		return ToStreamlineMatrix(stored);
	}

	sl::float3 ToStreamlineFloat3(const DirectX::XMFLOAT3& source) noexcept
	{
		return sl::float3{source.x, source.y, source.z};
	}

	sl::float3 NormalizeToStreamlineFloat3(const DirectX::XMFLOAT3& source, DirectX::FXMVECTOR fallback) noexcept
	{
		const DirectX::XMVECTOR vector = DirectX::XMLoadFloat3(&source);
		const float lengthSq = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(vector));
		if (lengthSq <= 1.0e-8f)
		{
			DirectX::XMFLOAT3 storedFallback{};
			DirectX::XMStoreFloat3(&storedFallback, fallback);
			return ToStreamlineFloat3(storedFallback);
		}

		DirectX::XMFLOAT3 normalized{};
		DirectX::XMStoreFloat3(&normalized, DirectX::XMVector3Normalize(vector));
		return ToStreamlineFloat3(normalized);
	}

	sl::float3 MatrixBasisRowToStreamlineFloat3(const DirectX::XMFLOAT4X4& matrix, std::size_t row) noexcept
	{
		const DirectX::XMFLOAT3 basis{matrix.m[row][0], matrix.m[row][1], matrix.m[row][2]};
		return NormalizeToStreamlineFloat3(basis, DirectX::XMVectorZero());
	}

	float CalculateVerticalFovRadians(const DirectX::XMFLOAT4X4& projection) noexcept
	{
		const float projectionYScale = projection.m[1][1];
		if (std::abs(projectionYScale) <= 1.0e-6f)
		{
			return 1.04719755f;
		}
		return 2.0f * std::atan(1.0f / projectionYScale);
	}

	float CalculateAspectRatio(const DirectX::XMFLOAT4X4& projection, const RenderViewportExtent& renderExtent) noexcept
	{
		const float projectionXScale = projection.m[0][0];
		const float projectionYScale = projection.m[1][1];
		if (std::abs(projectionXScale) > 1.0e-6f && std::abs(projectionYScale) > 1.0e-6f)
		{
			return projectionYScale / projectionXScale;
		}
		return renderExtent.Height > 0 ? static_cast<float>(renderExtent.Width) / static_cast<float>(renderExtent.Height) : 1.0f;
	}

	sl::float2 ConvertNdcJitterToPixelJitter(
	    const DirectX::XMFLOAT2& jitterNdc,
	    const RenderViewportExtent& renderExtent) noexcept
	{
		return sl::float2{
		    jitterNdc.x * static_cast<float>(renderExtent.Width) * 0.5f,
		    -jitterNdc.y * static_cast<float>(renderExtent.Height) * 0.5f};
	}

	sl::float2 BuildDlssMotionVectorScale(const UpscalerInputContract& inputContract) noexcept
	{
		float directionScale = 1.0f;
		if (inputContract.MotionVectorConvention.Direction == EUpscalerMotionVectorDirection::CurrentMinusPrevious)
		{
			directionScale = -1.0f;
		}

		return sl::float2{
		    inputContract.RenderExtent.Width > 0 ? directionScale / static_cast<float>(inputContract.RenderExtent.Width) : directionScale,
		    inputContract.RenderExtent.Height > 0 ? directionScale / static_cast<float>(inputContract.RenderExtent.Height) : directionScale};
	}

	void FillStreamlineConstants(sl::Constants& constants, const UpscalerInputContract& inputContract) noexcept
	{
		DirectX::XMMATRIX clipToPrevClip = DirectX::XMMatrixIdentity();
		DirectX::XMMATRIX prevClipToClip = DirectX::XMMatrixIdentity();
		if (inputContract.TemporalState.HistoryValid)
		{
			const DirectX::XMMATRIX invProjection = DirectX::XMLoadFloat4x4(&inputContract.Camera.InvProjectionMTX);
			const DirectX::XMMATRIX invView = DirectX::XMLoadFloat4x4(&inputContract.Camera.InvViewMTX);
			const DirectX::XMMATRIX previousViewProjection = DirectX::XMLoadFloat4x4(&inputContract.TemporalData.PrevViewProjMTX);
			clipToPrevClip = DirectX::XMMatrixMultiply(
			    DirectX::XMMatrixMultiply(invProjection, invView),
			    previousViewProjection);
			prevClipToClip = DirectX::XMMatrixInverse(nullptr, clipToPrevClip);
		}

		constants.cameraViewToClip = ToStreamlineMatrix(inputContract.Camera.ProjectionMTX);
		constants.clipToCameraView = ToStreamlineMatrix(inputContract.Camera.InvProjectionMTX);
		FillIdentity(constants.clipToLensClip);
		constants.clipToPrevClip = ToStreamlineMatrix(clipToPrevClip);
		constants.prevClipToClip = ToStreamlineMatrix(prevClipToClip);
		constants.jitterOffset = ConvertNdcJitterToPixelJitter(inputContract.TemporalState.JitterCurrent, inputContract.RenderExtent);
		constants.mvecScale = BuildDlssMotionVectorScale(inputContract);
		constants.cameraPinholeOffset = sl::float2{0.0f, 0.0f};
		constants.cameraPos = ToStreamlineFloat3(inputContract.Camera.Position);
		constants.cameraUp = MatrixBasisRowToStreamlineFloat3(inputContract.Camera.InvViewMTX, 1u);
		constants.cameraRight = MatrixBasisRowToStreamlineFloat3(inputContract.Camera.InvViewMTX, 0u);
		constants.cameraFwd = NormalizeToStreamlineFloat3(inputContract.Camera.Direction, DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f));
		constants.cameraNear = inputContract.Camera.NearZ;
		constants.cameraFar = inputContract.Camera.FarZ;
		constants.cameraFOV = CalculateVerticalFovRadians(inputContract.Camera.ProjectionMTX);
		constants.cameraAspectRatio = CalculateAspectRatio(inputContract.Camera.ProjectionMTX, inputContract.RenderExtent);
		constants.depthInverted =
		    inputContract.DepthConvention == EUpscalerDepthConvention::ReversedDeviceDepth ? sl::Boolean::eTrue : sl::Boolean::eFalse;
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

			const EUpscalerQualityMode frameQualityMode =
			    ResolveFrameQualityMode(m_qualityMode, evaluation.RenderExtent, evaluation.OutputExtent);
			m_diagnostics.SelectedQualityMode = frameQualityMode == m_qualityMode ?
			                                        UpscalerQualityModeToString(m_qualityMode) :
			                                        "NativeAA (forced until render extent scaling is wired)";

			sl::DLSSOptions options{};
			options.mode = ToStreamlineDlssMode(frameQualityMode);
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
			FillStreamlineConstants(constants, m_lastFrameContract);
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
