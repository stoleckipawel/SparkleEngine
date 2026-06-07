#pragma once

#include <cstdint>

struct RenderMeshWorkloadSummary final
{
	std::uint32_t staticInstanceCount = 0;
	std::uint32_t skinnedInstanceCount = 0;
	std::uint32_t staticBatchCount = 0;
	std::uint32_t skinnedBatchCount = 0;
	std::uint32_t jointMatrixCount = 0;
};
