#pragma once

#include "Scene/Materials/MaterialHandle.h"

#include <cstddef>

namespace MaterialHandleResolver
{
	std::uint32_t ResolveSlot(MaterialHandle handle, std::uint32_t materialGeneration, std::size_t materialCount);
}
