#pragma once

#include <cstdint>

struct ReferencePathTracerUniformData final
{
	static constexpr std::uint32_t WorkFlagTrace = 1u << 0u;
	static constexpr std::uint32_t WorkFlagClearDisplay = 1u << 1u;
	static constexpr std::uint32_t WorkFlagCommitPrefix = 1u << 2u;

	std::uint32_t SessionSeed = 0u;
	std::uint32_t ReplicateId = 0u;
	std::uint32_t SampleOrdinal = 0u;
	std::uint32_t FinitePathDiagnosticSurfaceVertices = 0u;
	std::uint32_t FirstRow = 0u;
	std::uint32_t RowCount = 0u;
	std::uint32_t PriorSampleCount = 0u;
	std::uint32_t WorkFlags = 0u;
};

static_assert(sizeof(ReferencePathTracerUniformData) == 32u);
