#include "PCH.h"

#include "Features/Instances/CookedSceneInstanceBuilder.h"

#include <cstdint>
#include <limits>

namespace
{
	Assets::CookedSceneInstanceGroupKind ToCookedInstanceGroupKind(ImportedMeshInstanceGroupKind groupKind) noexcept
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

	bool ResolveMaterialAssetIndex(
	    const ImportedMeshInstance& importedInstance,
	    const CookedSceneBuild& build,
	    std::uint32_t& outMaterialAssetIndex,
	    std::string& outErrorMessage)
	{
		outMaterialAssetIndex = Assets::kInvalidCookedMaterialAssetIndex;
		if (!importedInstance.HasMaterialBinding())
		{
			return true;
		}

		outMaterialAssetIndex = importedInstance.materialIndex;
		if (outMaterialAssetIndex >= build.outputs.materialAssets.size())
		{
			outErrorMessage = "Imported mesh instance references a material index outside the imported material set";
			return false;
		}

		return true;
	}

	bool ResolveMaterialAssetIndex(
	    const ImportedMeshInstanceGroup& importedGroup,
	    const CookedSceneBuild& build,
	    std::uint32_t& outMaterialAssetIndex,
	    std::string& outErrorMessage)
	{
		outMaterialAssetIndex = Assets::kInvalidCookedMaterialAssetIndex;
		if (!importedGroup.HasMaterialBinding())
		{
			return true;
		}

		outMaterialAssetIndex = importedGroup.materialIndex;
		if (outMaterialAssetIndex >= build.outputs.materialAssets.size())
		{
			outErrorMessage = "Imported mesh instance group references a material index outside the imported material set";
			return false;
		}

		return true;
	}

	bool SupportsMorphWeights(const ImportedMeshInstance& importedInstance, const CookedSceneBuild& build) noexcept
	{
		if (importedInstance.primitiveIndex >= build.manifest.meshAssetReferences.size())
		{
			return false;
		}

		return build.manifest.meshAssetReferences[importedInstance.primitiveIndex].meshAssetKind == Assets::CookedMeshAssetKind::Skeletal;
	}
}  // namespace

bool CookedSceneInstanceBuilder::BuildInstances(
    const SourceImportResult& importResult,
    CookedSceneBuild& build,
    std::string& outErrorMessage)
{
	build.manifest.instances.clear();
	build.manifest.instances.reserve(importResult.scene.meshInstances.size());
	build.manifest.instanceGroups.clear();
	build.manifest.instanceGroups.reserve(importResult.scene.meshInstanceGroups.size());

	for (std::size_t instanceIndex = 0; instanceIndex < importResult.scene.meshInstances.size(); ++instanceIndex)
	{
		const ImportedMeshInstance& importedInstance = importResult.scene.meshInstances[instanceIndex];
		if (!importedInstance.HasPrimitiveBinding() || importedInstance.primitiveIndex >= build.manifest.meshAssetReferences.size())
		{
			outErrorMessage = "Imported mesh instance references a primitive index outside the cooked mesh asset set";
			return false;
		}

		std::uint32_t materialAssetIndex = Assets::kInvalidCookedMaterialAssetIndex;
		if (!ResolveMaterialAssetIndex(importedInstance, build, materialAssetIndex, outErrorMessage))
		{
			return false;
		}

		std::uint32_t groupIndex = Assets::kInvalidCookedSceneInstanceGroupIndex;
		if (importedInstance.groupIndex != kInvalidImportedMeshInstanceGroupIndex)
		{
			if (importedInstance.groupIndex >= importResult.scene.meshInstanceGroups.size())
			{
				outErrorMessage = "Imported mesh instance references an instance group outside the imported group set";
				return false;
			}

			groupIndex = importedInstance.groupIndex;
		}

		std::uint32_t skeletonRefIndex = Assets::kInvalidCookedSceneSkeletonRefIndex;
		if (importedInstance.HasSkeletonBinding())
		{
			if (importedInstance.skeletonIndex >= build.manifest.skeletonRefs.size())
			{
				outErrorMessage = "Imported mesh instance references a skeleton outside the cooked skeleton set";
				return false;
			}

			skeletonRefIndex = importedInstance.skeletonIndex;
		}

		std::uint32_t firstMorphWeight = Assets::kInvalidCookedSceneMorphWeightIndex;
		std::uint32_t morphWeightCount = 0;
		if (!importedInstance.morphWeights.empty() && SupportsMorphWeights(importedInstance, build))
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

	for (std::size_t groupIndex = 0; groupIndex < importResult.scene.meshInstanceGroups.size(); ++groupIndex)
	{
		const ImportedMeshInstanceGroup& importedGroup = importResult.scene.meshInstanceGroups[groupIndex];
		if (!importedGroup.HasPrimitiveBinding() || importedGroup.primitiveIndex >= build.manifest.meshAssetReferences.size())
		{
			outErrorMessage = "Imported mesh instance group references a primitive index outside the cooked mesh asset set";
			return false;
		}

		std::uint32_t materialAssetIndex = Assets::kInvalidCookedMaterialAssetIndex;
		if (!ResolveMaterialAssetIndex(importedGroup, build, materialAssetIndex, outErrorMessage))
		{
			return false;
		}

		if (!importedGroup.HasInstanceRange() || importedGroup.firstInstanceIndex >= build.manifest.instances.size() ||
		    importedGroup.instanceCount > build.manifest.instances.size() - importedGroup.firstInstanceIndex)
		{
			outErrorMessage = "Imported mesh instance group references an instance range outside the cooked instance set";
			return false;
		}

		if (groupIndex > (std::numeric_limits<std::uint32_t>::max)())
		{
			outErrorMessage = "Imported mesh instance group count exceeds the cooked scene manifest range";
			return false;
		}

		build.manifest.instanceGroups.push_back(
		    Assets::CookedSceneInstanceGroupRecord{
		        .meshAssetIndex = importedGroup.primitiveIndex,
		        .materialAssetIndex = materialAssetIndex,
		        .firstInstance = importedGroup.firstInstanceIndex,
		        .instanceCount = importedGroup.instanceCount,
		        .groupKind = ToCookedInstanceGroupKind(importedGroup.groupKind),
		        .flags = importedGroup.flags});
	}

	outErrorMessage.clear();
	return true;
}
