#include "PCH.h"

#include "Scene/Preparation/PreparedRenderScene.h"

ResolvedMaterialTextureTable::operator bool() const noexcept
{
	return static_cast<bool>(Binding) && DescriptorCount != 0u && Generation != 0u;
}

void PreparedRenderScene::ResetForReuse() noexcept
{
	structuralRevision = 0;
	materialRevision = 0;
	directionalLights.Clear();
	pointLights.Clear();
	spotLights.Clear();
	rectLights.Clear();
	sky = {};
	primitives.clear();
	instanceGroups.clear();
	jointMatrices.clear();
	previousJointMatrices.clear();
	morphWeights.clear();
	previousMorphWeights.clear();
	rayTracingWork.BlasInputs.clear();
	rayTracingWork.ClassicTlasBlasInputIndices.clear();
	rayTracingWork.PartitionedTlasBlasInputIndices.clear();
	materialGeneration.reset();
	materials = {};
	materialTextureTable = {};
}
