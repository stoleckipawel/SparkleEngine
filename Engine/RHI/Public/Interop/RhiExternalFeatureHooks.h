#pragma once

#include "../Core/RhiBackendApi.h"
#include "RhiNativeHandles.h"

#include <cstdint>

enum class ERhiExternalInterfaceKind : std::uint8_t
{
	GraphicsDevice = 0,
	GraphicsQueue,
	PresentationFactory,
	PresentationSurface
};

using RhiExternalDeviceCreatedCallback = bool (*)(
    ERhiBackendApi backendApi,
    NativeGraphicsDeviceHandle nativeDevice,
    void* userData) noexcept;

// On replacement, UpgradeInterface transfers one native-interface reference to
// the caller. ResolveNativeInterface always transfers one reference.
using RhiExternalInterfaceUpgradeCallback = bool (*)(
    ERhiBackendApi backendApi,
    ERhiExternalInterfaceKind kind,
    void** nativeInterface,
    void* userData) noexcept;
using RhiExternalInterfaceResolveCallback = bool (*)(
    ERhiBackendApi backendApi,
    ERhiExternalInterfaceKind kind,
    void* externalInterface,
    void** nativeInterface,
    void* userData) noexcept;
using RhiExternalPresentationReadyCallback = void (*)(
    ERhiBackendApi backendApi,
    bool ready,
    void* userData) noexcept;

struct RhiExternalFeatureHooks final
{
	RhiExternalDeviceCreatedCallback DeviceCreated = nullptr;
	RhiExternalInterfaceUpgradeCallback UpgradeInterface = nullptr;
	RhiExternalInterfaceResolveCallback ResolveNativeInterface = nullptr;
	RhiExternalPresentationReadyCallback PresentationReady = nullptr;
	void* UserData = nullptr;
};
