#pragma once

#include "Providers/RenderProviderModel.h"
#include "Upscaling/UpscalerInputContract.h"
#include "Viewport/ViewportContracts.h"
#include "RHI/Public/Core/RhiBackendApi.h"
#include "RHI/Public/Interop/RhiInteropService.h"
#include "RHI/Public/Interop/RhiNativeHandles.h"

#include <cstdint>
#include <string>
#include <string_view>

struct RhiCapabilities;

// Renderer-owned provider boundary for image upscalers and external reconstruction
// features. Implementations translate renderer contracts to provider policy; RHI
// only supplies backend/native-handle capability facts through RhiCapabilities.
enum class EUpscalerProviderKind : std::uint8_t
{
	Passthrough = 0,
	NvidiaDlss = 1
};

enum class EUpscalerProviderFailureDomain : std::uint8_t
{
	None = 0,
	Sdk = 1,
	Driver = 2,
	Backend = 3,
	Feature = 4,
	ResourceState = 5,
	InputContract = 6
};

struct UpscalerProviderCapabilities final
{
	EUpscalerProviderKind Kind = EUpscalerProviderKind::Passthrough;
	ERendererProviderCategory Category = ERendererProviderCategory::Upscaler;
	ERendererProviderCapabilityState CapabilityState = ERendererProviderCapabilityState::Unavailable;
	EUpscalerProviderFailureDomain FailureDomain = EUpscalerProviderFailureDomain::None;
	bool CanInitialize = false;
	bool CanEvaluate = false;
	bool UsesExternalSdk = false;
	std::string ProviderName;
	RendererProviderResourceContract ResourceContract = {};
	std::string ResourceContractSummary;
	std::string ExternalRuntimeVersion;
	std::string RuntimeState;
	std::string SelectedQualityMode;
	std::string FeatureMatrixSummary;
	RenderViewportExtent RenderExtent = {};
	RenderViewportExtent OutputExtent = {};
	bool ResetRequested = false;
	std::string ResetReason;
	std::string Reason;
};

struct UpscalerPresentationBridge final
{
	using UpgradePresentationInterfaceFn = bool (*)(RhiNativeInterfaceUpgradeCallback callback, void* callbackUserData, void* bridgeUserData);

	UpgradePresentationInterfaceFn UpgradePresentationInterface = nullptr;
	void* UserData = nullptr;

	constexpr explicit operator bool() const noexcept { return UpgradePresentationInterface != nullptr; }
};

struct UpscalerEvaluationDesc final
{
	RenderProductHandle InputColor = {};
	RenderProductHandle Depth = {};
	RenderProductHandle MotionVectors = {};
	RenderProductHandle OutputColor = {};
	ERhiBackendApi BackendApi = ERhiBackendApi::Unknown;
	NativeGraphicsCommandListHandle NativeCommandList = {};
	NativeResourceHandle NativeInputColor = {};
	NativeResourceHandle NativeDepth = {};
	NativeResourceHandle NativeMotionVectors = {};
	NativeResourceHandle NativeOutputColor = {};
	NativeTextureViewInfo NativeInputColorView = {};
	NativeTextureViewInfo NativeDepthView = {};
	NativeTextureViewInfo NativeMotionVectorsView = {};
	NativeTextureViewInfo NativeOutputColorView = {};
	RenderViewportExtent RenderExtent = {};
	RenderViewportExtent OutputExtent = {};
	std::uint64_t FrameIndex = 0;
};

struct UpscalerEvaluationResult final
{
	bool ProducedOutput = false;
	bool UsedFallback = false;
	EUpscalerProviderFailureDomain FailureDomain = EUpscalerProviderFailureDomain::None;
	std::string Reason;
};

class IUpscalerProvider
{
  public:
	virtual ~IUpscalerProvider() = default;

	virtual EUpscalerProviderKind GetKind() const noexcept = 0;
	virtual std::string_view GetName() const noexcept = 0;
	virtual UpscalerProviderCapabilities QueryCapabilities(const RhiCapabilities& capabilities) const = 0;
	virtual bool Initialize(
	    const RhiCapabilities& capabilities,
	    RhiNativeDeviceQueueInterop nativeInterop,
	    UpscalerPresentationBridge presentationBridge) = 0;
	virtual void SetupFrame(const UpscalerInputContract& inputContract) = 0;
	virtual UpscalerEvaluationResult Evaluate(const UpscalerEvaluationDesc& evaluation) = 0;
	virtual void OnResize(RenderViewportExtent renderExtent, RenderViewportExtent outputExtent) = 0;
	virtual void ResetHistory(std::string_view reason) = 0;
	virtual void Shutdown() noexcept = 0;
	virtual UpscalerProviderCapabilities GetDiagnostics() const = 0;
};

const char* UpscalerProviderKindToString(EUpscalerProviderKind kind) noexcept;
const char* UpscalerProviderFailureDomainToString(EUpscalerProviderFailureDomain domain) noexcept;
