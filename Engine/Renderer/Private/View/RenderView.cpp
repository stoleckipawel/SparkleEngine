#include "PCH.h"

#include "View/RenderView.h"

void RenderView::ResetForReuse() noexcept
{
	std::vector<std::uint32_t> retainedRasterPrimitiveIndices = std::move(rasterPrimitiveIndices);
	std::vector<MeshInstanceBatch> retainedMeshInstanceBatches = std::move(meshInstanceBatches);
	*this = {};
	rasterPrimitiveIndices = std::move(retainedRasterPrimitiveIndices);
	meshInstanceBatches = std::move(retainedMeshInstanceBatches);
	rasterPrimitiveIndices.clear();
	meshInstanceBatches.clear();
}
