#include "PCH.h"

#include "SceneData/RenderSceneData.h"

ResolvedMaterialTextureTable::operator bool() const noexcept
{
	return static_cast<bool>(Binding) &&
	       DescriptorCount != 0u &&
	       Generation != 0u;
}

void RenderSceneData::ResetForReuse() noexcept
{
	structuralRevision = 0;
	materialRevision = 0;
	directionalLights.Clear();
	pointLights.Clear();
	spotLights.Clear();
	rectLights.Clear();
	sky = {};
	meshInstances.clear();
	meshWorldBounds.clear();
	rasterMeshInstanceIndices.clear();
	meshInstanceBatches.clear();
	jointMatrices.clear();
	previousJointMatrices.clear();
	morphWeights.clear();
	previousMorphWeights.clear();
	meshWorkload = {};
	rayTracingWork.BlasInputs.clear();
	rayTracingWork.ClassicTlasBlasInputIndices.clear();
	rayTracingWork.PartitionedTlasBlasInputIndices.clear();
	materials = {};
	materialTextureTable = {};
}
