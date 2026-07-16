#pragma once

#include <cstdint>

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

struct NativeResourceHandle
{
	void* Value = nullptr;

	constexpr explicit operator bool() const noexcept { return Value != nullptr; }
};

using NativeTextureHandle = NativeResourceHandle;

struct NativeTextureViewHandle
{
	void* Value = nullptr;

	constexpr explicit operator bool() const noexcept { return Value != nullptr; }
};

struct NativeTextureViewInfo
{
	NativeResourceHandle Resource = {};
	NativeTextureViewHandle View = {};
	std::uint32_t NativeState = 0;
	std::uint32_t NativeFormat = 0;
	std::uint32_t Width = 0;
	std::uint32_t Height = 0;
	std::uint32_t MipLevels = 1;
	std::uint32_t ArrayLayers = 1;
	std::uint32_t SubresourceAspectMask = 0;
	std::uint32_t SubresourceBaseMipLevel = 0;
	std::uint32_t SubresourceLevelCount = 1;
	std::uint32_t SubresourceBaseArrayLayer = 0;
	std::uint32_t SubresourceLayerCount = 1;
	std::uint32_t NativeFlags = 0;
	std::uint32_t NativeUsage = 0;

	constexpr explicit operator bool() const noexcept { return Resource && View && Width != 0 && Height != 0; }
};
