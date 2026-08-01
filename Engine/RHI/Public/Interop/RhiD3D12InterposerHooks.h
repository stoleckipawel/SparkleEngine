#pragma once

#include "../Presentation/RhiFrameLatencyMarker.h"
#include "RhiNativeHandles.h"

#include <cstdint>

struct RhiAdapterIdentity;

enum class ERhiD3D12InterposerInterfaceKind : std::uint8_t
{
	GraphicsDevice = 0,
	GraphicsQueue,
	PresentationFactory,
	PresentationSurface
};

using RhiD3D12DeviceCreatedCallback =
    bool (*)(NativeGraphicsDeviceHandle nativeDevice, const RhiAdapterIdentity& adapter, void* userData) noexcept;

// On replacement, UpgradeInterface transfers one native-interface reference to
// the caller. ResolveNativeInterface always transfers one reference.
using RhiD3D12InterfaceUpgradeCallback =
    bool (*)(ERhiD3D12InterposerInterfaceKind kind, void** nativeInterface, void* userData) noexcept;
using RhiD3D12InterfaceResolveCallback = bool (*)(
    ERhiD3D12InterposerInterfaceKind kind,
    void* externalInterface,
    void** nativeInterface,
    void* userData) noexcept;
using RhiD3D12PresentationReadyCallback = void (*)(bool ready, void* userData) noexcept;
using RhiD3D12FrameMarkerCallback =
    void (*)(ERhiFrameLatencyMarker marker, std::uint64_t frameId, void* userData) noexcept;
using RhiD3D12RuntimeShutdownCallback = void (*)(void* userData) noexcept;

struct RhiD3D12InterposerHooks final
{
	RhiD3D12DeviceCreatedCallback DeviceCreated = nullptr;
	RhiD3D12InterfaceUpgradeCallback UpgradeInterface = nullptr;
	RhiD3D12InterfaceResolveCallback ResolveNativeInterface = nullptr;
	RhiD3D12PresentationReadyCallback PresentationReady = nullptr;
	RhiD3D12FrameMarkerCallback FrameMarker = nullptr;
	RhiD3D12RuntimeShutdownCallback RuntimeShutdown = nullptr;
	void* UserData = nullptr;
};
