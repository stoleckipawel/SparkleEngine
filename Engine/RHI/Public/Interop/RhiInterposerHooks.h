#pragma once

#include "../Presentation/RhiFrameLatencyMarker.h"
#include "RhiNativeHandles.h"

#include <cstdint>

struct RhiAdapterIdentity;

enum class ERhiInterposerInterfaceKind : std::uint8_t
{
	GraphicsDevice = 0,
	GraphicsQueue,
	PresentationFactory,
	PresentationSurface
};

using RhiDeviceCreatedCallback =
    bool (*)(NativeGraphicsDeviceHandle nativeDevice, const RhiAdapterIdentity& adapter, void* userData) noexcept;

// On replacement, UpgradeInterface transfers one native-interface ownership
// reference to the caller. ResolveNativeInterface does the same.
using RhiInterfaceUpgradeCallback = bool (*)(ERhiInterposerInterfaceKind kind, void** nativeInterface, void* userData) noexcept;
using RhiInterfaceResolveCallback =
    bool (*)(ERhiInterposerInterfaceKind kind, void* externalInterface, void** nativeInterface, void* userData) noexcept;
using RhiPresentationReadyCallback = void (*)(bool ready, void* userData) noexcept;
using RhiFrameMarkerCallback = void (*)(ERhiFrameLatencyMarker marker, std::uint64_t frameId, void* userData) noexcept;
using RhiRuntimeShutdownCallback = void (*)(void* userData) noexcept;

struct RhiInterposerHooks final
{
	RhiDeviceCreatedCallback DeviceCreated = nullptr;
	RhiInterfaceUpgradeCallback UpgradeInterface = nullptr;
	RhiInterfaceResolveCallback ResolveNativeInterface = nullptr;
	RhiPresentationReadyCallback PresentationReady = nullptr;
	RhiFrameMarkerCallback FrameMarker = nullptr;
	RhiRuntimeShutdownCallback RuntimeShutdown = nullptr;
	void* UserData = nullptr;

	constexpr explicit operator bool() const noexcept
	{
		return DeviceCreated != nullptr || UpgradeInterface != nullptr || ResolveNativeInterface != nullptr || PresentationReady != nullptr
		    || FrameMarker != nullptr || RuntimeShutdown != nullptr;
	}
};
