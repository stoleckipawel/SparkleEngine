#pragma once

#include "RHI/Public/Core/RhiBackendApi.h"
#include "RHI/Public/Interop/RhiInteropService.h"
#include "RHI/Public/Interop/RhiNativeHandles.h"
#include "Viewport/ViewportContracts.h"

#include <cstdint>
struct ImageProviderFrameInput;
struct RhiCapabilities;
struct RayReconstructionEvaluationDesc final
{
	ERhiBackendApi BackendApi = ERhiBackendApi::Unknown;
	NativeGraphicsCommandListHandle NativeCommandList = {};
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
};

class IRayReconstructionProvider
{
public:
	virtual ~IRayReconstructionProvider() = default;
	IRayReconstructionProvider(const IRayReconstructionProvider&) = delete;
	IRayReconstructionProvider& operator=(const IRayReconstructionProvider&) = delete;
	IRayReconstructionProvider(IRayReconstructionProvider&&) = delete;
	IRayReconstructionProvider& operator=(IRayReconstructionProvider&&) = delete;

	virtual bool Initialize(const RhiCapabilities& capabilities, RhiNativeDeviceQueueInterop nativeInterop) = 0;
	virtual RenderViewportExtent ResolveRenderExtent(RenderViewportExtent outputExtent) noexcept = 0;
	virtual void SetupFrame(const ImageProviderFrameInput& frameInput) = 0;
	virtual bool Evaluate(const RayReconstructionEvaluationDesc& evaluation) = 0;
	virtual void Shutdown() noexcept = 0;

protected:
	IRayReconstructionProvider() = default;
};
