#pragma once

#include <cstdint>

struct ReferencePathTracerUniformData final
{
	std::uint32_t SessionSeed = 0u;
	std::uint32_t ReplicateId = 0u;
	std::uint32_t SampleOrdinal = 0u;
	std::uint32_t FinitePathDiagnosticSurfaceVertices = 0u;
};

static_assert(sizeof(ReferencePathTracerUniformData) == 16u);
