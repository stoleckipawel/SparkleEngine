#pragma once

#include <cstddef>
#include <functional>
#include <string_view>

struct ShaderCookProgress final
{
	std::string_view Phase;
	std::size_t Completed = 0;
	std::size_t Total = 0;
};

using ShaderCookProgressCallback = std::function<void(const ShaderCookProgress&)>;
