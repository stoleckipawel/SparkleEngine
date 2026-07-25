#include "PCH.h"
#include "Renderer/Public/Editor/EditorTextureHandle.h"

EditorTextureHandle::operator bool() const noexcept
{
	return Slot != 0 && Generation != 0;
}

std::uint64_t EditorTextureHandle::Pack() const noexcept
{
	return (static_cast<std::uint64_t>(Generation) << 32u) | Slot;
}

EditorTextureHandle EditorTextureHandle::Unpack(std::uint64_t value) noexcept
{
	return EditorTextureHandle{
	    .Slot = static_cast<std::uint32_t>(value),
	    .Generation = static_cast<std::uint32_t>(value >> 32u)};
}

EditorTextureHandle EditorTextureHandle::FontAtlas() noexcept
{
	return EditorTextureHandle{.Slot = 1, .Generation = 1};
}

EditorTextureHandle EditorTextureHandle::Viewport(std::uint64_t generation) noexcept
{
	if (generation == 0 || generation > UINT32_MAX)
	{
		return {};
	}
	return EditorTextureHandle{
	    .Slot = 2,
	    .Generation = static_cast<std::uint32_t>(generation)};
}

bool EditorTextureHandle::operator==(const EditorTextureHandle& other) const noexcept
{
	return Slot == other.Slot && Generation == other.Generation;
}
