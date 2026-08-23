#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class ShaderRecookRequestType : std::uint8_t
{
	Global = 0,
	Changed,
	ShaderId,
};

struct ShaderRecookRequest final
{
	ShaderRecookRequestType Type = ShaderRecookRequestType::Global;
	std::string Target;
	std::vector<std::string> ChangedVirtualPaths;
};
