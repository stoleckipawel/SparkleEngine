#include "PCH.h"

#include "Scene/Meshes/SceneMorphWeightApplicator.h"

#include "Scene/Meshes/MeshComponent.h"
#include "Scene/Meshes/SkeletalCookedMesh.h"

namespace
{
	void ApplyToMatchingSkeletalMesh(const SceneMorphWeightSnapshot& morphWeight, const std::unique_ptr<MeshComponent>& meshComponent)
	{
		if (!meshComponent || !meshComponent->IsSkeletalMeshComponent() || meshComponent->GetSourceNodeIndex() != morphWeight.targetNodeIndex)
		{
			return;
		}

		if (auto* skeletalMesh = dynamic_cast<SkeletalCookedMesh*>(meshComponent->GetMesh()))
		{
			skeletalMesh->SetMorphWeights(morphWeight.weights);
		}
	}
}

namespace SceneMorphWeightApplicator
{
	void Apply(
	    std::span<const SceneMorphWeightSnapshot> morphWeights,
	    const std::vector<std::unique_ptr<MeshComponent>>& meshComponents)
	{
		for (const SceneMorphWeightSnapshot& morphWeight : morphWeights)
		{
			for (const std::unique_ptr<MeshComponent>& meshComponent : meshComponents)
			{
				ApplyToMatchingSkeletalMesh(morphWeight, meshComponent);
			}
		}
	}
}
