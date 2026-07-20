#pragma once

#include "World/ECS/Components/TransformComponents.h"

#include <cstdint>

namespace ECS
{
	struct OscillatingMotion final
	{
		LocalTransform BaseTransform;
		std::uint32_t LaneIndex = 0;
	};
}
