#include "PCH.h"

#include "Assets/Loaders/SceneManifestInstanceGroupValidator.h"

#include "Assets/Cooked/LoadedSceneManifest.h"

#include <cstddef>
#include <format>

namespace Assets::SceneManifestInstanceGroupValidator
{
	namespace
	{
		bool ValidateReferences(const LoadedSceneManifest& manifest, std::size_t groupIndex, std::string& outErrorMessage)
		{
			const CookedSceneInstanceGroupRecord& group = manifest.instanceGroups[groupIndex];
			if (group.meshAssetIndex >= manifest.meshAssetReferences.size())
			{
				outErrorMessage = std::format(
				    "Cooked scene instance group {} references mesh asset index {} but only {} mesh assets exist",
				    groupIndex,
				    group.meshAssetIndex,
				    manifest.meshAssetReferences.size());
				return false;
			}

			if (group.materialAssetIndex != kInvalidCookedMaterialAssetIndex && group.materialAssetIndex >= manifest.materialAssetReferences.size())
			{
				outErrorMessage = std::format(
				    "Cooked scene instance group {} references material asset index {} but only {} material assets exist",
				    groupIndex,
				    group.materialAssetIndex,
				    manifest.materialAssetReferences.size());
				return false;
			}

			return true;
		}

		bool ValidateRange(const LoadedSceneManifest& manifest, std::size_t groupIndex, std::string& outErrorMessage)
		{
			const CookedSceneInstanceGroupRecord& group = manifest.instanceGroups[groupIndex];
			if (group.instanceCount == 0 || group.firstInstance >= manifest.instances.size() ||
			    group.instanceCount > manifest.instances.size() - group.firstInstance)
			{
				outErrorMessage = std::format(
				    "Cooked scene instance group {} references invalid instance range first={} count={} with {} instances",
				    groupIndex,
				    group.firstInstance,
				    group.instanceCount,
				    manifest.instances.size());
				return false;
			}

			for (std::uint32_t instanceOffset = 0; instanceOffset < group.instanceCount; ++instanceOffset)
			{
				const std::size_t instanceIndex = static_cast<std::size_t>(group.firstInstance) + instanceOffset;
				if (manifest.instances[instanceIndex].groupIndex != groupIndex)
				{
					outErrorMessage = std::format(
					    "Cooked scene instance group {} range contains instance {} with mismatched group index {}",
					    groupIndex,
					    instanceIndex,
					    manifest.instances[instanceIndex].groupIndex);
					return false;
				}
			}

			return true;
		}

		bool ValidateKind(const LoadedSceneManifest& manifest, std::size_t groupIndex, std::string& outErrorMessage)
		{
			const CookedSceneInstanceGroupRecord& group = manifest.instanceGroups[groupIndex];
			if (group.groupKind != CookedSceneInstanceGroupKind::None &&
			    group.groupKind != CookedSceneInstanceGroupKind::SharedMeshReference &&
			    group.groupKind != CookedSceneInstanceGroupKind::AuthoredInstanceGroup)
			{
				outErrorMessage = std::format("Cooked scene instance group {} uses an unknown group kind", groupIndex);
				return false;
			}

			return true;
		}
	}  // namespace

	bool Validate(const LoadedSceneManifest& manifest, std::string& outErrorMessage)
	{
		for (std::size_t groupIndex = 0; groupIndex < manifest.instanceGroups.size(); ++groupIndex)
		{
			if (!ValidateReferences(manifest, groupIndex, outErrorMessage) ||
			    !ValidateRange(manifest, groupIndex, outErrorMessage) ||
			    !ValidateKind(manifest, groupIndex, outErrorMessage))
			{
				return false;
			}
		}

		return true;
	}
}
