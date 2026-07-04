#pragma once

#include "RHI/Public/Core/RhiCapabilities.h"
#include "RHI/Public/Interop/RhiInteropService.h"

#include <cstdint>
#include <array>
#include <string>
#include <string_view>

#if SPARKLE_WITH_NVIDIA_STREAMLINE
#include <sl.h>

struct StreamlineBackendContract final
{
	bool Valid = false;
	bool UsesD3D12 = false;
	bool UsesVulkan = false;
	std::string FailureReason;
};

struct StreamlineAdapterInfo final
{
	sl::AdapterInfo Info = {};
	std::array<std::uint8_t, 8> LuidStorage = {};
};

bool HasStreamlineNativeAdapterLuid(const RhiAdapterIdentity& adapter) noexcept;
StreamlineBackendContract ValidateStreamlineBackend(
    const RhiCapabilities& capabilities,
    RhiNativeDeviceQueueInterop nativeInterop);
sl::Result SetStreamlineNativeDevice(
    const StreamlineBackendContract& backend,
    RhiNativeDeviceQueueInterop nativeInterop) noexcept;
StreamlineAdapterInfo BuildStreamlineAdapterInfo(
    const StreamlineBackendContract& backend,
    const RhiCapabilities& capabilities,
    RhiNativeDeviceQueueInterop nativeInterop) noexcept;
std::string FormatStreamlineFailure(std::string_view operation, sl::Result result);
bool UpgradePresentationInterfaceWithStreamline(void** nativeInterface, void*) noexcept;
void FillStreamlinePreferences(
    sl::Preferences& preferences,
    const sl::Feature* features,
    std::uint32_t featureCount,
    sl::RenderAPI renderApi);
#endif
