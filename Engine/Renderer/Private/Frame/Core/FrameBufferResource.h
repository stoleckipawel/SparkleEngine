#pragma once

#include "RHI/Public/Interop/RhiNativeHandles.h"

#include <cstdint>

struct FrameBufferResource final
{
	RhiOwnedResourceHandle Resource = {};
	std::uint64_t SizeInBytes = 0;
	std::uint32_t StrideInBytes = 0;

	bool IsValid() const noexcept { return Resource && SizeInBytes > 0 && StrideInBytes > 0; }
	explicit operator bool() const noexcept { return IsValid(); }
	void Reset() noexcept { *this = {}; }
};
