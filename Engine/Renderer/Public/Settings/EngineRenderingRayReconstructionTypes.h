#pragma once

#include <cstdint>

enum class EngineRayReconstructionMode : std::uint8_t
{
	Off,
	NvidiaDlssRayReconstruction,
};
