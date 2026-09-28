#pragma once

#include <cstddef>
#include <functional>
#include <string_view>

struct ToolWorkProgressEvent final
{
	std::string_view Action;
	std::string_view Item;
	std::size_t Completed = 0;
	std::size_t Total = 0;
};

using ToolWorkProgressCallback = std::function<void(const ToolWorkProgressEvent&)>;
