#pragma once

#include <cstdint>
#include <string>

enum class ShaderRecookRequestType : std::uint8_t
{
	Global = 0,
	Changed,
	PackageId,
	ShaderId,
};

struct ShaderRecookRequest final
{
	ShaderRecookRequestType Type = ShaderRecookRequestType::Global;
	std::string Target;
};
