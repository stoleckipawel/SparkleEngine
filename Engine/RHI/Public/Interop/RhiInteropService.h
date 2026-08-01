#pragma once

#include "../Core/RhiBackendApi.h"
#include "../Interop/RhiNativeHandles.h"
#include "../Resources/RhiResourceView.h"
#include "../RHIAPI.h"

#include <cstdint>

enum class ERhiNativeInteropConsumer : std::uint8_t
{
	Unknown = 0,
	Diagnostics,
	ExternalProvider,
	Presentation
};

struct RhiNativeInteropRequest final
{
	ERhiNativeInteropConsumer Consumer = ERhiNativeInteropConsumer::Unknown;
	const char* Reason = "";
};

constexpr bool IsRhiNativeInteropRequestValid(const RhiNativeInteropRequest& request) noexcept
{
	return request.Consumer != ERhiNativeInteropConsumer::Unknown && request.Reason != nullptr && request.Reason[0] != '\0';
}

struct RhiNativeDeviceQueueInterop final
{
	ERhiBackendApi BackendApi = ERhiBackendApi::Unknown;
	NativeGraphicsDeviceHandle Device = {};
	NativeGraphicsQueueHandle GraphicsQueue = {};
	RhiNativeInteropRequest Request = {};

	constexpr explicit operator bool() const noexcept { return Device.Value != nullptr && GraphicsQueue.Value != nullptr; }
};

class SPARKLE_RHI_API RhiInteropService
{
  public:
	virtual ~RhiInteropService() noexcept = default;

	virtual RhiNativeDeviceQueueInterop GetDeviceQueueInterop(RhiNativeInteropRequest request) const noexcept = 0;
	virtual NativeTextureViewInfo GetNativeTextureViewInfo(
	    RhiResourceViewHandle view,
	    RhiResourceHandle resource,
	    ResourceState state,
	    const RhiNativeInteropRequest& request) const noexcept = 0;
};
