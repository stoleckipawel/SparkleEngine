#pragma once

#include "Scene/Materials/MaterialDesc.h"
#include "Scene/Materials/MaterialHandle.h"
#include "Scene/Materials/MaterialSnapshot.h"

#include <cstddef>
#include <vector>

namespace MaterialCacheUtils
{
	std::uint32_t ResolveMaterialSlot(MaterialHandle materialHandle, std::size_t materialCount);
	bool MaterialSnapshotEquals(const MaterialSnapshot& left, const MaterialSnapshot& right);
}  // namespace MaterialCacheUtils
