#pragma once

#include "RayTracing/Effects/IndirectDiffuse/IndirectDiffuseDebugMode.h"

#include <cstdint>

struct IndirectDiffuseSettings final
{
	IndirectDiffuseDebugMode DebugMode = IndirectDiffuseDebugMode::Off;
	float NormalBias = 0.01f;
	float MaxDistance = 100000.0f;
	float Intensity = 1.0f;
	std::uint32_t BounceCount = 1u;
};

IndirectDiffuseSettings BuildIndirectDiffuseSettings() noexcept;
