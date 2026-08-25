#pragma once

#include "RayTracing/RhiRayTracingPipelineDesc.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

struct RhiRayTracingShaderTablePackingRules final
{
	std::uint64_t IdentifierSizeInBytes = 0;
	std::uint64_t RecordAlignmentInBytes = 0;
	std::uint64_t TableAlignmentInBytes = 0;
	std::uint64_t MaximumRecordStrideInBytes = 0;
};

namespace RhiRayTracingShaderTablePacking
{
	RhiRayTracingShaderTableRegion AppendRegion(
	    std::span<const RhiRayTracingShaderRecord> records,
	    std::span<const std::byte> identifiers,
	    const RhiRayTracingShaderTablePackingRules& rules,
	    std::vector<std::byte>& bytes);
}
