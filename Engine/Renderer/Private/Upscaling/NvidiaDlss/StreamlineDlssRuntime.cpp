#include "../../PCH.h"
#include "Upscaling/NvidiaDlss/StreamlineDlssRuntime.h"
#include "Upscaling/NvidiaDlss/StreamlineDlssConstants.h"
#include "Upscaling/NvidiaDlss/StreamlineDlssFeatureMatrix.h"
#include "Upscaling/NvidiaDlss/StreamlineDlssResourceTags.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
#include <vulkan/vulkan.h>
#include <sl.h>
#include <sl_dlss.h>
#include <sl_helpers.h>
#include <sl_helpers_vk.h>
#endif

#include <array>
#include <filesystem>
#include <format>
#include <system_error>

namespace
{
	bool HasNativeAdapterLuid(const RhiAdapterIdentity& adapter) noexcept
	{
		return adapter.NativeLuidSizeInBytes > 0 && adapter.NativeLuidSizeInBytes <= adapter.NativeLuid.size();
	}

	bool HasNativeEvaluationContract(const UpscalerEvaluationDesc& evaluation) noexcept
	{
		const bool hasNativeResources =
		    evaluation.NativeCommandList && evaluation.NativeScalingInputColor && evaluation.NativeDepth && evaluation.NativeMotionVectors &&
		    evaluation.NativeScalingOutputColor;
		if (evaluation.BackendApi != ERhiBackendApi::Vulkan)
		{
			return hasNativeResources;
		}

		return hasNativeResources && evaluation.NativeScalingInputColorView && evaluation.NativeDepthView &&
		       evaluation.NativeMotionVectorsView && evaluation.NativeScalingOutputColorView;
	}

#if SPARKLE_WITH_NVIDIA_STREAMLINE
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

	sl::DLSSOptions BuildDlssOptions(EUpscalerQualityMode qualityMode, RenderViewportExtent outputExtent) noexcept
	{
		sl::DLSSOptions options{};
		options.mode = ToStreamlineDlssMode(qualityMode);
		options.outputWidth = outputExtent.Width;
		options.outputHeight = outputExtent.Height;
		options.colorBuffersHDR = sl::Boolean::eTrue;
		options.useAutoExposure = sl::Boolean::eTrue;
		options.alphaUpscalingEnabled = sl::Boolean::eFalse;
		options.dlaaPreset = sl::DLSSPreset::ePresetK;
		options.qualityPreset = sl::DLSSPreset::ePresetK;
		options.balancedPreset = sl::DLSSPreset::ePresetK;
		options.performancePreset = sl::DLSSPreset::ePresetM;
		options.ultraPerformancePreset = sl::DLSSPreset::ePresetL;
		return options;
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
			m_diagnostics.SdkVersion = kStreamlineDlssSdkVersion;
			m_diagnostics.SelectedQualityMode = UpscalerQualityModeToString(desc.QualityMode);

			const bool isD3D12 = desc.Capabilities.BackendApi == ERhiBackendApi::D3D12;
			const bool isVulkan = desc.Capabilities.BackendApi == ERhiBackendApi::Vulkan;
			if (!isD3D12 && !isVulkan)
			{
				m_diagnostics.State = EDlssProviderRuntimeState::Unavailable;
				m_diagnostics.FailureDomain = EUpscalerProviderFailureDomain::Backend;
				m_diagnostics.FailureReason = "Streamline DLSS is only implemented for D3D12 and Vulkan backends.";
				m_diagnostics.FeatureMatrix = CreateStreamlineDlssFeatureMatrix(false, m_diagnostics.FailureReason);
				return false;
			}
			if (isD3D12 && !desc.NativeInterop.Device)
			{
				m_diagnostics.State = EDlssProviderRuntimeState::Unavailable;
				m_diagnostics.FailureDomain = EUpscalerProviderFailureDomain::Backend;
				m_diagnostics.FailureReason = "D3D12 native device handle is unavailable.";
				m_diagnostics.FeatureMatrix = CreateStreamlineDlssFeatureMatrix(false, m_diagnostics.FailureReason);
				return false;
			}
			if (isVulkan && (!desc.Capabilities.ExternalFeatureInterop.VulkanInstance ||
			                 !desc.Capabilities.ExternalFeatureInterop.VulkanPhysicalDevice ||
			                 !desc.Capabilities.ExternalFeatureInterop.VulkanDevice ||
			                 desc.Capabilities.ExternalFeatureInterop.VulkanGraphicsQueueFamilyIndex == UINT32_MAX))
			{
				m_diagnostics.State = EDlssProviderRuntimeState::Unavailable;
				m_diagnostics.FailureDomain = EUpscalerProviderFailureDomain::Backend;
				m_diagnostics.FailureReason = "Vulkan native instance, physical device, device, or graphics queue family is unavailable.";
				m_diagnostics.FeatureMatrix = CreateStreamlineDlssFeatureMatrix(false, m_diagnostics.FailureReason);
				return false;
			}
			if (isD3D12 && !HasNativeAdapterLuid(desc.Capabilities.ExternalFeatureInterop.Adapter))
			{
				m_diagnostics.State = EDlssProviderRuntimeState::Unavailable;
				m_diagnostics.FailureDomain = EUpscalerProviderFailureDomain::Backend;
				m_diagnostics.FailureReason = "D3D12 adapter native LUID is unavailable for Streamline feature support query.";
				m_diagnostics.FeatureMatrix = CreateStreamlineDlssFeatureMatrix(false, m_diagnostics.FailureReason);
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
			preferences.renderAPI = isVulkan ? sl::RenderAPI::eVulkan : sl::RenderAPI::eD3D12;
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
				m_diagnostics.FailureDomain = EUpscalerProviderFailureDomain::Sdk;
				m_diagnostics.FailureReason = FormatStreamlineFailure("slInit", result);
				m_diagnostics.FeatureMatrix = CreateStreamlineDlssFeatureMatrix(false, m_diagnostics.FailureReason);
				return false;
			}
			m_initialized = true;

			if (isD3D12 && desc.PresentationBridge &&
			    !desc.PresentationBridge.UpgradePresentationInterface(
			        &UpgradePresentationInterfaceWithStreamline,
			        nullptr,
			        desc.PresentationBridge.UserData))
			{
				m_diagnostics.State = EDlssProviderRuntimeState::Unavailable;
				m_diagnostics.FailureDomain = EUpscalerProviderFailureDomain::Sdk;
				m_diagnostics.FailureReason = "Streamline failed to upgrade the RHI presentation interface for manual present hooks.";
				m_diagnostics.FeatureMatrix = CreateStreamlineDlssFeatureMatrix(false, m_diagnostics.FailureReason);
				return false;
			}

			if (isD3D12)
			{
				result = slSetD3DDevice(desc.NativeInterop.Device.Value);
				if (result != sl::Result::eOk)
				{
					m_diagnostics.State = EDlssProviderRuntimeState::Unavailable;
					m_diagnostics.FailureDomain = EUpscalerProviderFailureDomain::Sdk;
					m_diagnostics.FailureReason = FormatStreamlineFailure("slSetD3DDevice", result);
					m_diagnostics.FeatureMatrix = CreateStreamlineDlssFeatureMatrix(false, m_diagnostics.FailureReason);
					return false;
				}
			}
			else
			{
				sl::VulkanInfo vulkanInfo{};
				vulkanInfo.instance = static_cast<VkInstance>(desc.Capabilities.ExternalFeatureInterop.VulkanInstance);
				vulkanInfo.physicalDevice = static_cast<VkPhysicalDevice>(desc.Capabilities.ExternalFeatureInterop.VulkanPhysicalDevice);
				vulkanInfo.device = static_cast<VkDevice>(desc.Capabilities.ExternalFeatureInterop.VulkanDevice);
				vulkanInfo.graphicsQueueFamily = desc.Capabilities.ExternalFeatureInterop.VulkanGraphicsQueueFamilyIndex;
				vulkanInfo.computeQueueFamily = desc.Capabilities.ExternalFeatureInterop.VulkanGraphicsQueueFamilyIndex;
				vulkanInfo.graphicsQueueIndex = 0;
				vulkanInfo.computeQueueIndex = 0;
				result = slSetVulkanInfo(vulkanInfo);
				if (result != sl::Result::eOk)
				{
					m_diagnostics.State = EDlssProviderRuntimeState::Unavailable;
					m_diagnostics.FailureDomain = EUpscalerProviderFailureDomain::Sdk;
					m_diagnostics.FailureReason = FormatStreamlineFailure("slSetVulkanInfo", result);
					m_diagnostics.FeatureMatrix = CreateStreamlineDlssFeatureMatrix(false, m_diagnostics.FailureReason);
					return false;
				}
			}

			std::array<std::uint8_t, 8> adapterLuid = desc.Capabilities.ExternalFeatureInterop.Adapter.NativeLuid;
			sl::AdapterInfo adapterInfo{};
			if (isVulkan)
			{
				adapterInfo.vkPhysicalDevice = desc.Capabilities.ExternalFeatureInterop.VulkanPhysicalDevice;
			}
			else
			{
				adapterInfo.deviceLUID = adapterLuid.data();
				adapterInfo.deviceLUIDSizeInBytes = desc.Capabilities.ExternalFeatureInterop.Adapter.NativeLuidSizeInBytes;
			}
			result = slIsFeatureSupported(sl::kFeatureDLSS, adapterInfo);
			if (result != sl::Result::eOk)
			{
				m_diagnostics.State = EDlssProviderRuntimeState::Unavailable;
				m_diagnostics.FailureDomain = EUpscalerProviderFailureDomain::Feature;
				m_diagnostics.FailureReason = FormatStreamlineFailure("slIsFeatureSupported(DLSS)", result);
				m_diagnostics.FeatureMatrix = CreateStreamlineDlssFeatureMatrix(false, m_diagnostics.FailureReason);
				return false;
			}

			m_viewport = sl::ViewportHandle{0u};
			m_diagnostics.State = EDlssProviderRuntimeState::Created;
			m_diagnostics.FailureDomain = EUpscalerProviderFailureDomain::None;
			m_diagnostics.FeatureMatrix = CreateStreamlineDlssFeatureMatrix(true, "DLSS Super Resolution is supported by Streamline.");
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
				m_diagnostics.FailureDomain = EUpscalerProviderFailureDomain::Sdk;
				return {
				    .ProducedOutput = false,
				    .UsedFallback = true,
				    .FailureDomain = m_diagnostics.FailureDomain,
				    .Reason = "Streamline DLSS runtime is not initialized."};
			}
			if (!HasNativeEvaluationContract(evaluation))
			{
				m_diagnostics.State = EDlssProviderRuntimeState::FailedWithFallback;
				m_diagnostics.FailureDomain = EUpscalerProviderFailureDomain::InputContract;
				m_diagnostics.FailureReason = "DLSS evaluation contract is missing a native command list or required native resources.";
				return {
				    .ProducedOutput = false,
				    .UsedFallback = true,
				    .FailureDomain = m_diagnostics.FailureDomain,
				    .Reason = m_diagnostics.FailureReason};
			}

			const std::uint32_t frameIndex = static_cast<std::uint32_t>(m_lastFrameContract.FrameIndex);
			sl::FrameToken* frameToken = nullptr;
			sl::Result result = slGetNewFrameToken(frameToken, &frameIndex);
			if (result != sl::Result::eOk || frameToken == nullptr)
			{
				m_diagnostics.State = EDlssProviderRuntimeState::FailedWithFallback;
				m_diagnostics.FailureDomain = EUpscalerProviderFailureDomain::Sdk;
				m_diagnostics.FailureReason = FormatStreamlineFailure("slGetNewFrameToken", result);
				return {
				    .ProducedOutput = false,
				    .UsedFallback = true,
				    .FailureDomain = m_diagnostics.FailureDomain,
				    .Reason = m_diagnostics.FailureReason};
			}

			const EUpscalerQualityMode frameQualityMode =
			    ResolveFrameQualityMode(m_qualityMode, evaluation.RenderExtent, evaluation.OutputExtent);
			m_diagnostics.SelectedQualityMode = frameQualityMode == m_qualityMode ?
			                                        UpscalerQualityModeToString(m_qualityMode) :
			                                        "NativeAA (forced until render extent scaling is wired)";

			sl::DLSSOptions options = BuildDlssOptions(frameQualityMode, evaluation.OutputExtent);
			result = slDLSSSetOptions(m_viewport, options);
			if (result != sl::Result::eOk)
			{
				m_diagnostics.State = EDlssProviderRuntimeState::FailedWithFallback;
				m_diagnostics.FailureDomain = EUpscalerProviderFailureDomain::Sdk;
				m_diagnostics.FailureReason = FormatStreamlineFailure("slDLSSSetOptions", result);
				return {
				    .ProducedOutput = false,
				    .UsedFallback = true,
				    .FailureDomain = m_diagnostics.FailureDomain,
				    .Reason = m_diagnostics.FailureReason};
			}

			sl::Constants constants{};
			FillStreamlineConstants(constants, m_lastFrameContract);
			result = slSetConstants(constants, *frameToken, m_viewport);
			if (result != sl::Result::eOk)
			{
				m_diagnostics.State = EDlssProviderRuntimeState::FailedWithFallback;
				m_diagnostics.FailureDomain = EUpscalerProviderFailureDomain::Sdk;
				m_diagnostics.FailureReason = FormatStreamlineFailure("slSetConstants", result);
				return {
				    .ProducedOutput = false,
				    .UsedFallback = true,
				    .FailureDomain = m_diagnostics.FailureDomain,
				    .Reason = m_diagnostics.FailureReason};
			}

			auto* commandBuffer = static_cast<sl::CommandBuffer*>(evaluation.NativeCommandList.Value);
			result = TagDlssResourcesForFrame(*frameToken, m_viewport, evaluation);
			if (result != sl::Result::eOk)
			{
				m_diagnostics.State = EDlssProviderRuntimeState::FailedWithFallback;
				m_diagnostics.FailureDomain = EUpscalerProviderFailureDomain::Sdk;
				m_diagnostics.FailureReason = FormatStreamlineFailure("slSetTagForFrame(DLSS)", result);
				return {
				    .ProducedOutput = false,
				    .UsedFallback = true,
				    .FailureDomain = m_diagnostics.FailureDomain,
				    .Reason = m_diagnostics.FailureReason};
			}

			const sl::BaseStructure* inputs[] = {&m_viewport};
			result = slEvaluateFeature(sl::kFeatureDLSS, *frameToken, inputs, static_cast<std::uint32_t>(std::size(inputs)), commandBuffer);
			if (result != sl::Result::eOk)
			{
				m_diagnostics.State = EDlssProviderRuntimeState::FailedWithFallback;
				m_diagnostics.FailureDomain = EUpscalerProviderFailureDomain::Sdk;
				m_diagnostics.FailureReason = FormatStreamlineFailure("slEvaluateFeature(DLSS)", result);
				return {
				    .ProducedOutput = false,
				    .UsedFallback = true,
				    .FailureDomain = m_diagnostics.FailureDomain,
				    .Reason = m_diagnostics.FailureReason};
			}

			m_diagnostics.State = EDlssProviderRuntimeState::Evaluating;
			m_diagnostics.FailureDomain = EUpscalerProviderFailureDomain::None;
			m_diagnostics.FailureReason.clear();
			return {
			    .ProducedOutput = true,
			    .UsedFallback = false,
			    .FailureDomain = EUpscalerProviderFailureDomain::None,
			    .Reason = "Streamline DLSS evaluated successfully."};
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
			m_diagnostics.FeatureMatrix = CreateUnavailableStreamlineDlssFeatureMatrix(kStreamlineDlssNotIntegratedReason);
			m_diagnostics.FailureDomain = EUpscalerProviderFailureDomain::Sdk;
			m_diagnostics.FailureReason = kStreamlineDlssNotIntegratedReason;
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
			m_diagnostics.FailureDomain =
			    HasNativeEvaluationContract(evaluation) ? EUpscalerProviderFailureDomain::Sdk : EUpscalerProviderFailureDomain::InputContract;
			m_diagnostics.FailureReason =
			    HasNativeEvaluationContract(evaluation)
			        ? kStreamlineDlssNotIntegratedReason
			        : "DLSS evaluation contract is missing a native command list or required native resources.";
			return UpscalerEvaluationResult{
			    .ProducedOutput = false,
			    .UsedFallback = true,
			    .FailureDomain = m_diagnostics.FailureDomain,
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

std::unique_ptr<IStreamlineDlssRuntime> CreateStreamlineDlssRuntime()
{
#if SPARKLE_WITH_NVIDIA_STREAMLINE
	return std::make_unique<StreamlineDlssRuntime>();
#else
	return std::make_unique<UnavailableStreamlineDlssRuntime>();
#endif
}
