#pragma once

#include "../RendererAPI.h"

#include <cstdint>

struct SPARKLE_RENDERER_API EditorTextureHandle final
{
	std::uint32_t Slot = 0;
	std::uint32_t Generation = 0;

	explicit operator bool() const noexcept;
	std::uint64_t Pack() const noexcept;
	static EditorTextureHandle Unpack(std::uint64_t value) noexcept;
	static EditorTextureHandle ImGuiTexture(std::uint32_t uniqueId) noexcept;
	static EditorTextureHandle Viewport(std::uint64_t generation) noexcept;
	bool IsImGuiTexture() const noexcept;
	bool operator==(const EditorTextureHandle& other) const noexcept;
};
