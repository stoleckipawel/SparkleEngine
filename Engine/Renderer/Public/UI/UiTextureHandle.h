#pragma once

#include "../RendererAPI.h"

#include <cstdint>

struct SPARKLE_RENDERER_API UiTextureHandle final
{
	std::uint32_t Slot = 0;
	std::uint32_t Generation = 0;

	explicit operator bool() const noexcept;
	std::uint64_t Pack() const noexcept;
	static UiTextureHandle Unpack(std::uint64_t value) noexcept;
	static UiTextureHandle ImGuiTexture(std::uint32_t uniqueId) noexcept;
	static UiTextureHandle Viewport(std::uint64_t generation) noexcept;
	bool IsImGuiTexture() const noexcept;
	bool operator==(const UiTextureHandle& other) const noexcept;
};
