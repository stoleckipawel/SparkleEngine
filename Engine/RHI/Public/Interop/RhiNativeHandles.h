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

struct NativeDescriptorHeapHandle
{
	void* Value = nullptr;

	constexpr explicit operator bool() const noexcept { return Value != nullptr; }
};

struct NativeResourceHandle
{
	void* Value = nullptr;

	constexpr explicit operator bool() const noexcept { return Value != nullptr; }
};

using NativeTextureHandle = NativeResourceHandle;

struct RhiOwnedHeapHandle
{
	void* Value = nullptr;

	constexpr explicit operator bool() const noexcept { return Value != nullptr; }
};

struct RhiOwnedResourceHandle
{
	void* Value = nullptr;

	constexpr explicit operator bool() const noexcept { return Value != nullptr; }
};
