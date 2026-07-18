#pragma once

#include <cstdint>
#include <string>

namespace ECS
{
	struct Name final
	{
		std::string Value;
	};

	struct EditorMetadata final
	{
		std::uint64_t AuthoredObjectId = 0;
		bool Selectable = true;
		bool Locked = false;
		bool HiddenInHierarchy = false;
	};
}
