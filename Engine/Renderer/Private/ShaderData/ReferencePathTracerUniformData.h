#pragma once

#include <cstdint>

struct ReferencePathTracerUniformData final
{
	std::uint32_t SessionSeed = 0u;
	std::uint32_t ReplicateId = 0u;
	std::uint32_t SampleOrdinal = 0u;
	std::uint32_t MaximumSurfaceVertices = 2u;
	float ContinuationNormalBiasMeters = 0.001f;
	std::uint32_t Padding[3] = {};
};

static_assert(sizeof(ReferencePathTracerUniformData) == 32u);
