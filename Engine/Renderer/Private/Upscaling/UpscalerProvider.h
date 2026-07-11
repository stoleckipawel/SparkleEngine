#pragma once

#include "Viewport/ViewportContracts.h"
#include "RHI/Public/Core/RhiBackendApi.h"
#include "RHI/Public/Interop/RhiInteropService.h"
#include "RHI/Public/Interop/RhiNativeHandles.h"

#include <cstdint>
struct ImageProviderFrameContext;
struct RhiCapabilities;

struct UpscalerEvaluationDesc final
{
	ERhiBackendApi BackendApi = ERhiBackendApi::Unknown;
	NativeGraphicsCommandListHandle NativeCommandList = {};
	NativeTextureViewInfo NativeScalingInputColorView = {};
	NativeTextureViewInfo NativeDepthView = {};
	NativeTextureViewInfo NativeMotionVectorsView = {};
	NativeTextureViewInfo NativeExposureView = {};
	NativeTextureViewInfo NativeScalingOutputColorView = {};
	RenderViewportExtent RenderExtent = {};
	RenderViewportExtent OutputExtent = {};
};

class IUpscalerProvider
{
  public:
	virtual ~IUpscalerProvider() = default;

	virtual bool Initialize(const RhiCapabilities& capabilities, RhiNativeDeviceQueueInterop nativeInterop) = 0;
	virtual RenderViewportExtent ResolveRenderExtent(RenderViewportExtent outputExtent) noexcept = 0;
	virtual void SetupFrame(const ImageProviderFrameContext& frameContext) = 0;
	virtual bool Evaluate(const UpscalerEvaluationDesc& evaluation) = 0;
	virtual void Shutdown() noexcept = 0;
};
