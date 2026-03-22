#pragma once

#include "GameFramework/Public/Assets/MaterialDesc.h"
#include "GameFramework/Public/Scene/Materials/MaterialSnapshot.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace MaterialCacheUtils
{
	std::uint32_t ResolveMaterialId(std::uint32_t materialId, std::size_t materialCount);
	bool MaterialSnapshotEquals(const MaterialSnapshot& left, const MaterialSnapshot& right);
}  // namespace MaterialCacheUtils