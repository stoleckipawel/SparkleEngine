#pragma once

#include "../RHIAPI.h"

#include <cstdint>

enum class CompareOp : std::uint8_t
{
	Never = 0,
	Less,
	Equal,
	LessOrEqual,
	Greater,
	NotEqual,
	GreaterOrEqual,
	Always,
};