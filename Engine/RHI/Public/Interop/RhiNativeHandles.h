#pragma once

struct NativeGraphicsDeviceHandle
{
	void* Value = nullptr;

	constexpr explicit operator bool() const noexcept { return Value != nullptr; }
};

struct NativeGraphicsQueueHandle
{
	void* Value = nullptr;

	constexpr explicit operator bool() const noexcept { return Value != nullptr; }
};

struct NativeGraphicsCommandListHandle
{
	void* Value = nullptr;

	constexpr explicit operator bool() const noexcept { return Value != nullptr; }
};

using RhiNativeInterfaceUpgradeCallback = bool (*)(void** nativeInterface, void* userData);

struct NativeResourceHandle
{
	void* Value = nullptr;

	constexpr explicit operator bool() const noexcept { return Value != nullptr; }
};

using NativeTextureHandle = NativeResourceHandle;

struct RhiOwnedMemoryBlockHandle
{
	void* Value = nullptr;

	constexpr explicit operator bool() const noexcept { return Value != nullptr; }
};

struct RhiOwnedResourceHandle
{
	void* Value = nullptr;

	constexpr explicit operator bool() const noexcept { return Value != nullptr; }
};
