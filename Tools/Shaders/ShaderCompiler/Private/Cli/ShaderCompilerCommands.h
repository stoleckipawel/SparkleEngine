#pragma once

#include <ostream>
#include <span>
#include <string_view>

struct ShaderCompilerCommand final
{
	std::string_view Verb;
	int (*Run)(std::span<const std::string_view> args);
	std::string_view Usage;
};

const ShaderCompilerCommand* FindShaderCompilerCommand(std::string_view verb) noexcept;
void PrintShaderCompilerUsage(std::ostream& output);
