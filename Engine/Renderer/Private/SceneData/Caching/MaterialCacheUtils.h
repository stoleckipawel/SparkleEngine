#pragma once

#include "Scene/Materials/MaterialDesc.h"
#include "Scene/Materials/MaterialHandle.h"

#include <cstddef>

namespace MaterialCacheUtils
{
	std::uint32_t ResolveMaterialSlot(MaterialHandle materialHandle, std::size_t materialCount);
}  // namespace MaterialCacheUtils
