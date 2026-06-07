#include "PCH.h"

#include "Assets/Loaders/SceneManifestValidator.h"

#include "Assets/Cooked/LoadedSceneManifest.h"

#include <cstddef>
#include <format>

namespace Assets
{
	namespace
	{
		bool HasFeatureFlag(std::uint32_t flags, CookedSceneFeatureFlags feature) noexcept
		{
			return (flags & ToCookedSceneFeatureFlagMask(feature)) != 0u;
		}
	}  // namespace

	bool SceneManifestValidator::ValidateHeader(const LoadedSceneManifest& manifest, std::string& outErrorMessage)
	{
		if (manifest.header.fileHeader.magic != kCookedSceneManifestMagic)
		{
			outErrorMessage = "Invalid cooked scene manifest magic";
			return false;
		}

		if (manifest.header.fileHeader.version != kCookedSceneManifestVersion)
		{
			outErrorMessage = std::format(
			    "Cooked scene manifest version {} is not supported by this runtime; expected version {}. Recook the scene asset.",
			    manifest.header.fileHeader.version,
			    kCookedSceneManifestVersion);
			return false;
		}

		outErrorMessage.clear();
		return true;
	}

	bool SceneManifestValidator::ValidateRecords(const LoadedSceneManifest& manifest, std::string& outErrorMessage)
	{
		constexpr std::uint32_t knownFeatureFlags =
		    ToCookedSceneFeatureFlagMask(CookedSceneFeatureFlags::Cameras) |
		    ToCookedSceneFeatureFlagMask(CookedSceneFeatureFlags::Lights) |
		    ToCookedSceneFeatureFlagMask(CookedSceneFeatureFlags::Skeletons) |
		    ToCookedSceneFeatureFlagMask(CookedSceneFeatureFlags::Animations) |
		    ToCookedSceneFeatureFlagMask(CookedSceneFeatureFlags::SkinnedMeshes) |
		    ToCookedSceneFeatureFlagMask(CookedSceneFeatureFlags::MorphTargets) |
		    ToCookedSceneFeatureFlagMask(CookedSceneFeatureFlags::MaterialVariants) |
		    ToCookedSceneFeatureFlagMask(CookedSceneFeatureFlags::AuthoredMeshInstancing);
		if ((manifest.header.featureFlags & ~knownFeatureFlags) != 0u)
		{
			outErrorMessage = std::format("Cooked scene manifest uses unknown feature flag bits 0x{:08X}", manifest.header.featureFlags);
			return false;
		}

		if (!manifest.cameras.empty() && !HasFeatureFlag(manifest.header.featureFlags, CookedSceneFeatureFlags::Cameras))
		{
			outErrorMessage = "Cooked scene manifest has camera records but is missing the Cameras feature flag";
			return false;
		}

		if (!manifest.lights.empty() && !HasFeatureFlag(manifest.header.featureFlags, CookedSceneFeatureFlags::Lights))
		{
			outErrorMessage = "Cooked scene manifest has light records but is missing the Lights feature flag";
			return false;
		}

		if (!manifest.skeletonRefs.empty() && !HasFeatureFlag(manifest.header.featureFlags, CookedSceneFeatureFlags::Skeletons))
		{
			outErrorMessage = "Cooked scene manifest has skeleton refs but is missing the Skeletons feature flag";
			return false;
		}

		if (!manifest.animationRefs.empty() && !HasFeatureFlag(manifest.header.featureFlags, CookedSceneFeatureFlags::Animations))
		{
			outErrorMessage = "Cooked scene manifest has animation refs but is missing the Animations feature flag";
			return false;
		}

		for (std::size_t meshReferenceIndex = 0; meshReferenceIndex < manifest.meshAssetReferences.size(); ++meshReferenceIndex)
		{
			if (manifest.meshAssetReferences[meshReferenceIndex].meshAssetId == InvalidCookedAssetId)
			{
				outErrorMessage = std::format("Cooked scene manifest has an invalid mesh asset reference at index {}", meshReferenceIndex);
				return false;
			}
		}

		for (std::size_t instanceIndex = 0; instanceIndex < manifest.instances.size(); ++instanceIndex)
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
		}

		for (std::size_t groupIndex = 0; groupIndex < manifest.instanceGroups.size(); ++groupIndex)
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

			if (group.groupKind != CookedSceneInstanceGroupKind::None &&
			    group.groupKind != CookedSceneInstanceGroupKind::SharedMeshReference &&
			    group.groupKind != CookedSceneInstanceGroupKind::AuthoredInstanceGroup)
			{
				outErrorMessage = std::format("Cooked scene instance group {} uses an unknown group kind", groupIndex);
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
		}

		for (std::size_t cameraIndex = 0; cameraIndex < manifest.cameras.size(); ++cameraIndex)
		{
			const CookedSceneCameraRecord& camera = manifest.cameras[cameraIndex];
			if (camera.projectionKind != CookedSceneCameraProjectionKind::Perspective &&
			    camera.projectionKind != CookedSceneCameraProjectionKind::Orthographic &&
			    camera.projectionKind != CookedSceneCameraProjectionKind::Unknown)
			{
				outErrorMessage = std::format("Cooked scene camera {} uses an unknown projection kind", cameraIndex);
				return false;
			}
		}

		for (std::size_t lightIndex = 0; lightIndex < manifest.lights.size(); ++lightIndex)
		{
			const CookedSceneLightRecord& light = manifest.lights[lightIndex];
			if (light.kind != CookedSceneLightKind::Directional &&
			    light.kind != CookedSceneLightKind::Point &&
			    light.kind != CookedSceneLightKind::Spot &&
			    light.kind != CookedSceneLightKind::Unknown)
			{
				outErrorMessage = std::format("Cooked scene light {} uses an unknown light kind", lightIndex);
				return false;
			}
		}

		for (std::size_t skeletonIndex = 0; skeletonIndex < manifest.skeletonRefs.size(); ++skeletonIndex)
		{
			if (manifest.skeletonRefs[skeletonIndex].skeletonAssetId == InvalidCookedAssetId)
			{
				outErrorMessage = std::format("Cooked scene skeleton ref {} has an invalid asset id", skeletonIndex);
				return false;
			}
		}

		for (std::size_t animationIndex = 0; animationIndex < manifest.animationRefs.size(); ++animationIndex)
		{
			if (manifest.animationRefs[animationIndex].animationAssetId == InvalidCookedAssetId)
			{
				outErrorMessage = std::format("Cooked scene animation ref {} has an invalid asset id", animationIndex);
				return false;
			}
		}

		outErrorMessage.clear();
		return true;
	}
}
