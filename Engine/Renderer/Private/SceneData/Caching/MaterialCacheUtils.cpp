#include "PCH.h"
#include "MaterialCacheUtils.h"

namespace MaterialCacheUtils
{
	std::uint32_t ResolveMaterialSlot(MaterialHandle materialHandle, std::size_t materialCount)
	{
		const std::uint32_t materialSlot = materialHandle.IsValid() ? materialHandle.GetIndex() : 0;

		if (materialSlot < materialCount)
		{
			return materialSlot;
		}

		return 0;
	}
}  // namespace MaterialCacheUtils
