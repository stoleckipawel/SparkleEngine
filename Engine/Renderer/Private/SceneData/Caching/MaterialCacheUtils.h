#pragma once

#include "Scene/Materials/MaterialDesc.h"
#include "Scene/Materials/MaterialHandle.h"
#include "Rendering/RenderInputFrame.h"

#include <cstddef>
#include <vector>

namespace MaterialCacheUtils
{
	std::uint32_t ResolveMaterialSlot(MaterialHandle materialHandle, std::size_t materialCount);
	bool MaterialTableEquals(const RenderMaterialTable& left, const RenderMaterialTable& right);
}  // namespace MaterialCacheUtils
