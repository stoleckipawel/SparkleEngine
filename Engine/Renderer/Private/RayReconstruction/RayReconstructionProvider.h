#pragma once

#include "Providers/RenderProviderModel.h"
#include "RayReconstruction/RayReconstructionInputContract.h"
#include "RayReconstruction/RayReconstructionSettings.h"
#include "RHI/Public/Core/RhiBackendApi.h"
#include "RHI/Public/Core/RhiCapabilities.h"
#include "RHI/Public/Interop/RhiInteropService.h"
#include "RHI/Public/Interop/RhiNativeHandles.h"
#include "Viewport/ViewportContracts.h"

#include <cstdint>
#include <string>
#include <string_view>

enum class ERayReconstructionProviderKind : std::uint8_t
{
	None = 0,
	NvidiaDlssRayReconstruction = 1,
};

enum class ERayReconstructionProviderFailureDomain : std::uint8_t
{
	None = 0,
	Sdk = 1,
	Driver = 2,
	Backend = 3,
	Feature = 4,
	ResourceState = 5,
	InputContract = 6,
};

struct RayReconstructionProviderCapabilities final
{
	ERayReconstructionProviderKind Kind = ERayReconstructionProviderKind::None;
	ERendererProviderCategory Category = ERendererProviderCategory::RayReconstruction;
	ERendererProviderCapabilityState CapabilityState = ERendererProviderCapabilityState::Unavailable;
	ERayReconstructionProviderFailureDomain FailureDomain = ERayReconstructionProviderFailureDomain::None;
	bool CanInitialize = false;
	bool CanEvaluate = false;
	bool UsesExternalSdk = false;
	std::string ProviderName;
	RendererProviderRayReconstructionResourceContract ResourceContract = {};
	std::string ResourceContractSummary;
	std::string ExternalRuntimeVersion;
	std::string RuntimeState;
	std::string SelectedQualityMode;
	RenderViewportExtent RenderExtent = {};
	RenderViewportExtent OutputExtent = {};
	bool ResetRequested = false;
	std::string ResetReason;
	std::string Reason;
};

struct RayReconstructionPresentationBridge final
{
	using UpgradePresentationInterfaceFn = bool (*)(RhiNativeInterfaceUpgradeCallback callback, void* callbackUserData, void* bridgeUserData);

	UpgradePresentationInterfaceFn UpgradePresentationInterface = nullptr;
	void* UserData = nullptr;

	constexpr explicit operator bool() const noexcept { return UpgradePresentationInterface != nullptr; }
};

using ExternalProviderCommandStateResetFn = void (*)(void* userData) noexcept;

struct RayReconstructionEvaluationDesc final
{
	RenderProductHandle NoisyInputColor = {};
	RenderProductHandle OutputColor = {};
	RenderProductHandle Depth = {};
	RenderProductHandle MotionVectors = {};
	RenderProductHandle Exposure = {};
	RenderProductHandle Normals = {};
	RenderProductHandle Roughness = {};
	RenderProductHandle DiffuseAlbedo = {};
	RenderProductHandle SpecularAlbedo = {};
	RenderProductHandle SpecularHitDistance = {};
	ERhiBackendApi BackendApi = ERhiBackendApi::Unknown;
	NativeGraphicsCommandListHandle NativeCommandList = {};
	NativeResourceHandle NativeNoisyInputColor = {};
	NativeResourceHandle NativeOutputColor = {};
	NativeResourceHandle NativeDepth = {};
	NativeResourceHandle NativeMotionVectors = {};
	NativeResourceHandle NativeExposure = {};
	NativeResourceHandle NativeNormals = {};
	NativeResourceHandle NativeRoughness = {};
	NativeResourceHandle NativeDiffuseAlbedo = {};
	NativeResourceHandle NativeSpecularAlbedo = {};
	NativeResourceHandle NativeSpecularHitDistance = {};
	NativeTextureViewInfo NativeNoisyInputColorView = {};
	NativeTextureViewInfo NativeOutputColorView = {};
	NativeTextureViewInfo NativeDepthView = {};
	NativeTextureViewInfo NativeMotionVectorsView = {};
	NativeTextureViewInfo NativeExposureView = {};
	NativeTextureViewInfo NativeNormalsView = {};
	NativeTextureViewInfo NativeRoughnessView = {};
	NativeTextureViewInfo NativeDiffuseAlbedoView = {};
	NativeTextureViewInfo NativeSpecularAlbedoView = {};
	NativeTextureViewInfo NativeSpecularHitDistanceView = {};
	RenderViewportExtent RenderExtent = {};
	RenderViewportExtent OutputExtent = {};
	ExternalProviderCommandStateResetFn ResetCommandState = nullptr;
	void* ResetCommandStateUserData = nullptr;
};

struct RayReconstructionEvaluationResult final
{
	bool ProducedOutput = false;
	bool UsedFallback = false;
	ERayReconstructionProviderFailureDomain FailureDomain = ERayReconstructionProviderFailureDomain::None;
	std::string Reason;
};

class IRayReconstructionProvider
{
  public:
	virtual ~IRayReconstructionProvider() = default;

	virtual ERayReconstructionProviderKind GetKind() const noexcept = 0;
	virtual std::string_view GetName() const noexcept = 0;
	virtual RayReconstructionProviderCapabilities QueryCapabilities(const RhiCapabilities& capabilities) const = 0;
	virtual bool Initialize(
	    const RhiCapabilities& capabilities,
	    RhiNativeDeviceQueueInterop nativeInterop,
	    RayReconstructionPresentationBridge presentationBridge) = 0;
	virtual void SetupFrame(const RayReconstructionInputContract& inputContract) = 0;
	virtual RayReconstructionEvaluationResult Evaluate(const RayReconstructionEvaluationDesc& evaluation) = 0;
	virtual void OnResize(RenderViewportExtent renderExtent, RenderViewportExtent outputExtent) = 0;
	virtual void ResetHistory(std::string_view reason) = 0;
	virtual void Shutdown() noexcept = 0;
	virtual RayReconstructionProviderCapabilities GetDiagnostics() const = 0;
};

const char* RayReconstructionProviderKindToString(ERayReconstructionProviderKind kind) noexcept;
const char* RayReconstructionProviderFailureDomainToString(ERayReconstructionProviderFailureDomain domain) noexcept;
