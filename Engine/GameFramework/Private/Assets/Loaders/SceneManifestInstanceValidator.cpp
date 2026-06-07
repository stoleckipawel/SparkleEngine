#include "PCH.h"

#include "Assets/Loaders/SceneManifestInstanceValidator.h"

#include "Assets/Cooked/LoadedSceneManifest.h"

#include <cstddef>
#include <format>

namespace Assets::SceneManifestInstanceValidator
{
	namespace
	{
		bool ValidateReferenceRanges(const LoadedSceneManifest& manifest, std::size_t instanceIndex, std::string& outErrorMessage)
		{
			const CookedSceneInstanceRecord& instance = manifest.instances[instanceIndex];
			if (instance.meshAssetIndex >= manifest.meshAssetReferences.size())
			{
				outErrorMessage = std::format(
				    "Cooked scene instance {} references mesh asset index {} but only {} mesh assets exist",
				    instanceIndex,
				    instance.meshAssetIndex,
				    manifest.meshAssetReferences.size());
				return false;
			}

			if (instance.materialAssetIndex != kInvalidCookedMaterialAssetIndex && instance.materialAssetIndex >= manifest.materialAssetReferences.size())
			{
				outErrorMessage = std::format(
				    "Cooked scene instance {} references material asset index {} but only {} material assets exist",
				    instanceIndex,
				    instance.materialAssetIndex,
				    manifest.materialAssetReferences.size());
				return false;
			}

			if (instance.groupIndex != kInvalidCookedSceneInstanceGroupIndex && instance.groupIndex >= manifest.instanceGroups.size())
			{
				outErrorMessage = std::format(
				    "Cooked scene instance {} references instance group index {} but only {} groups exist",
				    instanceIndex,
				    instance.groupIndex,
				    manifest.instanceGroups.size());
				return false;
			}

			if (instance.skeletonRefIndex != kInvalidCookedSceneSkeletonRefIndex && instance.skeletonRefIndex >= manifest.skeletonRefs.size())
			{
				outErrorMessage = std::format(
				    "Cooked scene instance {} references skeleton ref index {} but only {} skeleton refs exist",
				    instanceIndex,
				    instance.skeletonRefIndex,
				    manifest.skeletonRefs.size());
				return false;
			}

			if (instance.firstMorphWeight != kInvalidCookedSceneMorphWeightIndex)
			{
				if (instance.morphWeightCount == 0u ||
				    instance.firstMorphWeight >= manifest.morphWeights.size() ||
				    instance.morphWeightCount > manifest.morphWeights.size() - instance.firstMorphWeight)
				{
					outErrorMessage = std::format(
					    "Cooked scene instance {} references invalid morph weight range first={} count={} with {} weights",
					    instanceIndex,
					    instance.firstMorphWeight,
					    instance.morphWeightCount,
					    manifest.morphWeights.size());
					return false;
				}
			}
			else if (instance.morphWeightCount != 0u)
			{
				outErrorMessage = std::format("Cooked scene instance {} has morph weights without a valid first weight index", instanceIndex);
				return false;
			}

			return true;
		}

		bool ValidateMeshBinding(const LoadedSceneManifest& manifest, std::size_t instanceIndex, std::string& outErrorMessage)
		{
			const CookedSceneInstanceRecord& instance = manifest.instances[instanceIndex];
			const CookedSceneMeshAssetRef& meshReference = manifest.meshAssetReferences[instance.meshAssetIndex];
			if (meshReference.meshAssetKind == CookedMeshAssetKind::Skeletal &&
			    instance.skeletonRefIndex == kInvalidCookedSceneSkeletonRefIndex)
			{
				outErrorMessage = std::format("Cooked scene instance {} uses a skeletal mesh without a skeleton ref", instanceIndex);
				return false;
			}
			if (meshReference.meshAssetKind == CookedMeshAssetKind::Static &&
			    instance.skeletonRefIndex != kInvalidCookedSceneSkeletonRefIndex)
			{
				outErrorMessage = std::format("Cooked scene instance {} uses a static mesh with a skeleton ref", instanceIndex);
				return false;
			}

			return true;
		}
	}  // namespace

	bool Validate(const LoadedSceneManifest& manifest, std::string& outErrorMessage)
	{
		for (std::size_t instanceIndex = 0; instanceIndex < manifest.instances.size(); ++instanceIndex)
		{
			if (!ValidateReferenceRanges(manifest, instanceIndex, outErrorMessage) ||
			    !ValidateMeshBinding(manifest, instanceIndex, outErrorMessage))
			{
				return false;
			}
		}

		return true;
	}
}
