#include "PCH.h"

#include "Features/Instances/CookedSceneInstanceBuilder.h"

#include "Core/Public/Diagnostics/Error.h"

#include <cstdint>
#include <limits>

class CookedSceneInstanceTranslation final
{
  public:
	static Assets::CookedSceneInstanceGroupKind ToCookedInstanceGroupKind(ImportedMeshInstanceGroupKind groupKind) noexcept
	{
		switch (groupKind)
		{
			case ImportedMeshInstanceGroupKind::SharedMeshReference:
				return Assets::CookedSceneInstanceGroupKind::SharedMeshReference;
			case ImportedMeshInstanceGroupKind::AuthoredInstanceGroup:
				return Assets::CookedSceneInstanceGroupKind::AuthoredInstanceGroup;
			case ImportedMeshInstanceGroupKind::None:
			default:
				return Assets::CookedSceneInstanceGroupKind::None;
		}
	}

	static std::uint32_t ResolveMaterialAssetIndex(
	    const ImportedMeshInstance& importedInstance,
	    const CookedSceneBuild& build)
	{
		if (!importedInstance.HasMaterialBinding())
		{
			return Assets::kInvalidCookedMaterialAssetIndex;
		}

		if (importedInstance.materialIndex >= build.outputs.materialAssets.size())
		{
			throw Diagnostics::Error("Imported mesh instance references a material index outside the imported material set.");
		}

		return importedInstance.materialIndex;
	}

	static std::uint32_t ResolveMaterialAssetIndex(
	    const ImportedMeshInstanceGroup& importedGroup,
	    const CookedSceneBuild& build)
	{
		if (!importedGroup.HasMaterialBinding())
		{
			return Assets::kInvalidCookedMaterialAssetIndex;
		}

		if (importedGroup.materialIndex >= build.outputs.materialAssets.size())
		{
			throw Diagnostics::Error(
			    "Imported mesh instance group references a material index outside the imported material set.");
		}

		return importedGroup.materialIndex;
	}

	static bool SupportsMorphWeights(const ImportedMeshInstance& importedInstance, const CookedSceneBuild& build) noexcept
	{
		if (importedInstance.primitiveIndex >= build.manifest.meshAssetReferences.size())
		{
			return false;
		}

		return build.manifest.meshAssetReferences[importedInstance.primitiveIndex].meshAssetKind == Assets::CookedMeshAssetKind::Skeletal;
	}
};

void CookedSceneInstanceBuilder::BuildInstances(const SourceImportOutput& importOutput, CookedSceneBuild& build)
{
	build.manifest.instances.clear();
	build.manifest.instances.reserve(importOutput.scene.meshInstances.size());
	build.manifest.instanceGroups.clear();
	build.manifest.instanceGroups.reserve(importOutput.scene.meshInstanceGroups.size());

	for (std::size_t instanceIndex = 0; instanceIndex < importOutput.scene.meshInstances.size(); ++instanceIndex)
	{
		const ImportedMeshInstance& importedInstance = importOutput.scene.meshInstances[instanceIndex];
		if (!importedInstance.HasPrimitiveBinding() || importedInstance.primitiveIndex >= build.manifest.meshAssetReferences.size())
		{
			throw Diagnostics::Error(
			    "Imported mesh instance references a primitive index outside the cooked mesh asset set.");
		}

		const std::uint32_t materialAssetIndex =
		    CookedSceneInstanceTranslation::ResolveMaterialAssetIndex(importedInstance, build);

		std::uint32_t groupIndex = Assets::kInvalidCookedSceneInstanceGroupIndex;
		if (importedInstance.groupIndex != kInvalidImportedMeshInstanceGroupIndex)
		{
			if (importedInstance.groupIndex >= importOutput.scene.meshInstanceGroups.size())
			{
				throw Diagnostics::Error(
				    "Imported mesh instance references an instance group outside the imported group set.");
			}

			groupIndex = importedInstance.groupIndex;
		}

		std::uint32_t skeletonRefIndex = Assets::kInvalidCookedSceneSkeletonRefIndex;
		if (importedInstance.HasSkeletonBinding())
		{
			if (importedInstance.skeletonIndex >= build.manifest.skeletonRefs.size())
			{
				throw Diagnostics::Error(
				    "Imported mesh instance references a skeleton outside the cooked skeleton set.");
			}

			skeletonRefIndex = importedInstance.skeletonIndex;
		}

		std::uint32_t firstMorphWeight = Assets::kInvalidCookedSceneMorphWeightIndex;
		std::uint32_t morphWeightCount = 0;
		if (!importedInstance.morphWeights.empty() && CookedSceneInstanceTranslation::SupportsMorphWeights(importedInstance, build))
		{
			firstMorphWeight = static_cast<std::uint32_t>(build.manifest.morphWeights.size());
			morphWeightCount = static_cast<std::uint32_t>(importedInstance.morphWeights.size());
			build.manifest.morphWeights.insert(
			    build.manifest.morphWeights.end(),
			    importedInstance.morphWeights.begin(),
			    importedInstance.morphWeights.end());
		}

		build.manifest.instances.push_back(
		    Assets::CookedSceneInstanceRecord{
		        .meshAssetIndex = importedInstance.primitiveIndex,
		        .materialAssetIndex = materialAssetIndex,
		        .groupIndex = groupIndex,
		        .skeletonRefIndex = skeletonRefIndex,
		        .sourceNodeIndex = importedInstance.sourceNodeIndex,
		        .firstMorphWeight = firstMorphWeight,
		        .morphWeightCount = morphWeightCount,
		        .worldTransform = importedInstance.worldTransform});
	}

	for (std::size_t groupIndex = 0; groupIndex < importOutput.scene.meshInstanceGroups.size(); ++groupIndex)
	{
		const ImportedMeshInstanceGroup& importedGroup = importOutput.scene.meshInstanceGroups[groupIndex];
		if (!importedGroup.HasPrimitiveBinding() || importedGroup.primitiveIndex >= build.manifest.meshAssetReferences.size())
		{
			throw Diagnostics::Error(
			    "Imported mesh instance group references a primitive index outside the cooked mesh asset set.");
		}

		const std::uint32_t materialAssetIndex =
		    CookedSceneInstanceTranslation::ResolveMaterialAssetIndex(importedGroup, build);

		if (!importedGroup.HasInstanceRange() || importedGroup.firstInstanceIndex >= build.manifest.instances.size() ||
		    importedGroup.instanceCount > build.manifest.instances.size() - importedGroup.firstInstanceIndex)
		{
			throw Diagnostics::Error(
			    "Imported mesh instance group references an instance range outside the cooked instance set.");
		}

		if (groupIndex > (std::numeric_limits<std::uint32_t>::max)())
		{
			throw Diagnostics::Error("Imported mesh instance group count exceeds the cooked scene manifest range.");
		}

		build.manifest.instanceGroups.push_back(
		    Assets::CookedSceneInstanceGroupRecord{
		        .meshAssetIndex = importedGroup.primitiveIndex,
		        .materialAssetIndex = materialAssetIndex,
		        .firstInstance = importedGroup.firstInstanceIndex,
		        .instanceCount = importedGroup.instanceCount,
		        .groupKind = CookedSceneInstanceTranslation::ToCookedInstanceGroupKind(importedGroup.groupKind),
		        .flags = importedGroup.flags});
	}
}
