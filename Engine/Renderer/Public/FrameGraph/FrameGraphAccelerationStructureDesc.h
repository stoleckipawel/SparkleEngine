#pragma once

#include <string>
#include <string_view>

struct FrameGraphAccelerationStructureDesc
{
	std::string name;

	static FrameGraphAccelerationStructureDesc Create(std::string_view name) noexcept
	{
		return FrameGraphAccelerationStructureDesc{std::string(name)};
	}
};
