#pragma once

#include "../Core/RhiBackendApi.h"
#include "../Interop/RhiNativeHandles.h"
#include "../Resources/RhiResourceDesc.h"
#include "../Resources/RhiResourceView.h"
#include "../RHIAPI.h"

#include <cstdint>

enum class ERhiNativeInteropConsumer : std::uint8_t
{
	Unknown = 0,
	Validation = 1,
	UpscalerProvider = 2,
	RendererFrameGraph = 3,
	PresentationBridge = 4,
	RayReconstructionProvider = 5
};

struct RhiNativeInteropRequest final
{
	ERhiNativeInteropConsumer Consumer = ERhiNativeInteropConsumer::Unknown;
	const char* Reason = "";
};

struct RhiNativeDeviceQueueInterop final
{
	ERhiBackendApi BackendApi = ERhiBackendApi::Unknown;
	NativeGraphicsDeviceHandle Device = {};
	NativeGraphicsQueueHandle GraphicsQueue = {};
	RhiNativeInteropRequest Request = {};

	constexpr explicit operator bool() const noexcept { return Device.Value != nullptr; }
};

class SPARKLE_RHI_API RhiInteropService
{
  public:
	virtual ~RhiInteropService() noexcept = default;

	virtual RhiNativeDeviceQueueInterop GetDeviceQueueInterop(RhiNativeInteropRequest request) const noexcept = 0;
	virtual bool UpgradePresentationInterface(RhiNativeInterfaceUpgradeCallback callback, void* userData) noexcept = 0;
	virtual NativeTextureViewInfo GetNativeTextureViewInfo(RhiResourceViewHandle view, ResourceState state) const noexcept = 0;
};
