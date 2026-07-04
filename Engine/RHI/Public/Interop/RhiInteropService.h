#pragma once

#include "../Core/RhiBackendApi.h"
#include "../Interop/RhiNativeHandles.h"
#include "../RHIAPI.h"

#include <cstdint>

enum class ERhiNativeInteropConsumer : std::uint8_t
{
	Unknown = 0,
	Validation = 1,
	UpscalerProvider = 2,
	PresentationBridge = 4,
	RayReconstructionProvider = 5
};

struct RhiNativeInteropRequest final
{
	ERhiNativeInteropConsumer Consumer = ERhiNativeInteropConsumer::Unknown;
	const char* Reason = "";
};

struct RhiNativeVulkanDeviceQueueInterop final
{
	void* Instance = nullptr;
	void* PhysicalDevice = nullptr;
	void* Device = nullptr;
	void* GraphicsQueue = nullptr;
	std::uint32_t GraphicsQueueFamilyIndex = UINT32_MAX;

	constexpr explicit operator bool() const noexcept
	{
		return Instance != nullptr && PhysicalDevice != nullptr && Device != nullptr && GraphicsQueue != nullptr &&
		       GraphicsQueueFamilyIndex != UINT32_MAX;
	}
};

struct RhiNativeDeviceQueueInterop final
{
	ERhiBackendApi BackendApi = ERhiBackendApi::Unknown;
	NativeGraphicsDeviceHandle Device = {};
	NativeGraphicsQueueHandle GraphicsQueue = {};
	RhiNativeVulkanDeviceQueueInterop Vulkan = {};
	RhiNativeInteropRequest Request = {};

	constexpr explicit operator bool() const noexcept { return Device.Value != nullptr && GraphicsQueue.Value != nullptr; }
};

class SPARKLE_RHI_API RhiInteropService
{
  public:
	virtual ~RhiInteropService() noexcept = default;

	virtual RhiNativeDeviceQueueInterop GetDeviceQueueInterop(RhiNativeInteropRequest request) const noexcept = 0;
	virtual bool UpgradePresentationInterface(RhiNativeInterfaceUpgradeCallback callback, void* userData) noexcept = 0;
};
