#pragma once

#include <cstdint>
#include <string_view>

enum class PathRoot : uint8_t
{
	Any,
	Project,
	Engine,

	Count
};

constexpr std::string_view GetPathRootName(PathRoot root) noexcept
{
	switch (root)
	{
		case PathRoot::Any:
			return "Any";
		case PathRoot::Project:
			return "Project";
		case PathRoot::Engine:
			return "Engine";
		case PathRoot::Count:
		default:
			return "Unknown";
	}
}
