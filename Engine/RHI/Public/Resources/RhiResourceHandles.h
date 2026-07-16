#pragma once

// Backend-neutral execution identities. These values may be stored and passed by
// Renderer/RHI code, but they do not grant native API interop. Native handles are
// exposed separately through request-scoped interop contracts.
struct RhiResourceHandle
{
	void* Value = nullptr;

	constexpr explicit operator bool() const noexcept { return Value != nullptr; }
};

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
