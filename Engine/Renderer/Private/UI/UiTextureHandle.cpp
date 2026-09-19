#include "PCH.h"
#include "Renderer/Public/UI/UiTextureHandle.h"

UiTextureHandle::operator bool() const noexcept
{
	return Slot != 0 && Generation != 0;
}

std::uint64_t UiTextureHandle::Pack() const noexcept
{
	return (static_cast<std::uint64_t>(Generation) << 32u) | Slot;
}

UiTextureHandle UiTextureHandle::Unpack(std::uint64_t value) noexcept
{
	return UiTextureHandle{.Slot = static_cast<std::uint32_t>(value), .Generation = static_cast<std::uint32_t>(value >> 32u)};
}

UiTextureHandle UiTextureHandle::ImGuiTexture(std::uint32_t uniqueId) noexcept
{
	if (uniqueId == 0)
	{
		return {};
	}

	return UiTextureHandle{.Slot = 1, .Generation = uniqueId};
}

UiTextureHandle UiTextureHandle::Viewport(std::uint64_t generation) noexcept
{
	if (generation == 0 || generation > UINT32_MAX)
	{
		return {};
	}
	return UiTextureHandle{.Slot = 2, .Generation = static_cast<std::uint32_t>(generation)};
}

bool UiTextureHandle::IsImGuiTexture() const noexcept
{
	return Slot == 1 && Generation != 0;
}

bool UiTextureHandle::operator==(const UiTextureHandle& other) const noexcept
{
	return Slot == other.Slot && Generation == other.Generation;
}
