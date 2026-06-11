#pragma once

#include "Upscaling/UpscalerInputContract.h"
#include "Viewport/ViewportContracts.h"
#include "RHI/Public/Core/RhiBackendApi.h"
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

enum class EUpscalerProviderStatus : std::uint8_t
{
	Unavailable = 0,
	Available = 1,
	Active = 2,
	FailedWithFallback = 3
};

struct UpscalerProviderCapabilities final
{
	EUpscalerProviderKind Kind = EUpscalerProviderKind::Passthrough;
	EUpscalerProviderStatus Status = EUpscalerProviderStatus::Unavailable;
	bool CanInitialize = false;
	bool CanEvaluate = false;
	bool UsesExternalSdk = false;
	std::string ProviderName;
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
	RenderViewportExtent RenderExtent = {};
	RenderViewportExtent OutputExtent = {};
	std::uint64_t FrameIndex = 0;
};

struct UpscalerEvaluationResult final
{
	bool ProducedOutput = false;
	bool UsedFallback = false;
	std::string Reason;
};

class IUpscalerProvider
{
  public:
	virtual ~IUpscalerProvider() = default;

	// Provider implementations own SDK lifetime, settings translation, reset
	// policy, diagnostics, and fallback decisions. Generic frame code should
	// schedule through this interface and must not include provider SDK headers.
	virtual EUpscalerProviderKind GetKind() const noexcept = 0;
	virtual std::string_view GetName() const noexcept = 0;
	virtual UpscalerProviderCapabilities QueryCapabilities(const RhiCapabilities& capabilities) const = 0;
	virtual bool Initialize(const RhiCapabilities& capabilities, NativeGraphicsDeviceHandle nativeDevice) = 0;
	virtual void SetupFrame(const UpscalerInputContract& inputContract) = 0;
	virtual UpscalerEvaluationResult Evaluate(const UpscalerEvaluationDesc& evaluation) = 0;
	virtual void OnResize(RenderViewportExtent renderExtent, RenderViewportExtent outputExtent) = 0;
	virtual void ResetHistory(std::string_view reason) = 0;
	virtual void Shutdown() noexcept = 0;
	virtual UpscalerProviderCapabilities GetDiagnostics() const = 0;
};

const char* UpscalerProviderKindToString(EUpscalerProviderKind kind) noexcept;
const char* UpscalerProviderStatusToString(EUpscalerProviderStatus status) noexcept;
