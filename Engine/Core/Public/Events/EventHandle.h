#pragma once

#include "Core/Public/CoreAPI.h"

#include <cstdint>

struct SPARKLE_CORE_API EventHandle
{
	std::uint32_t Id = 0;

	bool IsValid() const noexcept { return Id != 0; }
	void Invalidate() noexcept { Id = 0; }

	bool operator==(const EventHandle& Other) const noexcept { return Id == Other.Id; }
	bool operator!=(const EventHandle& Other) const noexcept { return Id != Other.Id; }
};
