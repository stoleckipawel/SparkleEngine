#pragma once

#include "Core/Public/Console/CVar.h"

#include <cstdint>

extern ConsoleVariable<std::uint32_t> CVarRestirIndirectLightingBounceCount;
extern ConsoleVariable<bool> CVarRestirIndirectTemporalReuse;
extern ConsoleVariable<bool> CVarRestirIndirectSpatialReuse;

std::uint32_t ResolveRestirIndirectBounceCount() noexcept;
