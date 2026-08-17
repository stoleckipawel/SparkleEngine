#include "PCH.h"

#include "Assets/Loaders/SceneManifestValidator.h"

#include "Assets/Cooked/LoadedSceneManifest.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Strings/StringUtils.h"

#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>

namespace Assets
{
	class SceneManifestValidation final
	{
	public:
		template <std::size_t Capacity> static bool HasTerminatedName(const char (&name)[Capacity]) noexcept
		{
			return Strings::IsNullTerminated(std::span(name));
		}
		template <std::size_t Capacity> static bool HasValidName(const char (&name)[Capacity]) noexcept
		{
			return name[0] != '\0' && HasTerminatedName(name);
		}
		[[noreturn]] static void Invalid(std::string message) { throw Diagnostics::Error(std::move(message)); }
		static bool HasFeatureFlag(std::uint32_t flags, CookedSceneFeatureFlags feature) noexcept;
		static void ValidateFeatures(const LoadedSceneManifest& manifest);
		static void ValidateMeshReferences(const LoadedSceneManifest& manifest);
		static void ValidateInstanceReferenceRanges(const LoadedSceneManifest& manifest, std::size_t instanceIndex);
		static void ValidateInstanceMeshBinding(const LoadedSceneManifest& manifest, std::size_t instanceIndex);
		static void ValidateInstances(const LoadedSceneManifest& manifest);
		static void ValidateInstanceGroupReferences(const LoadedSceneManifest& manifest, std::size_t groupIndex);
		static void ValidateInstanceGroupRange(const LoadedSceneManifest& manifest, std::size_t groupIndex);
		static void ValidateInstanceGroupKind(const LoadedSceneManifest& manifest, std::size_t groupIndex);
		static void ValidateInstanceGroups(const LoadedSceneManifest& manifest);
		static void ValidateCameras(const LoadedSceneManifest& manifest);
		static void ValidateLights(const LoadedSceneManifest& manifest);
		static void ValidateSkeletonRefs(const LoadedSceneManifest& manifest);
		static void ValidateAnimationRefs(const LoadedSceneManifest& manifest);
		static void ValidateMetadata(const LoadedSceneManifest& manifest);
		static void ValidateMaterialVariants(const LoadedSceneManifest& manifest);
	};

	bool SceneManifestValidation::HasFeatureFlag(std::uint32_t flags, CookedSceneFeatureFlags feature) noexcept
	{
		return (flags & ToCookedSceneFeatureFlagMask(feature)) != 0u;
	}

	void SceneManifestValidation::ValidateFeatures(const LoadedSceneManifest& manifest)
	{
		constexpr std::uint32_t knownFeatureFlags = ToCookedSceneFeatureFlagMask(CookedSceneFeatureFlags::Cameras)
		    | ToCookedSceneFeatureFlagMask(CookedSceneFeatureFlags::Lights)
		    | ToCookedSceneFeatureFlagMask(CookedSceneFeatureFlags::Skeletons)
		    | ToCookedSceneFeatureFlagMask(CookedSceneFeatureFlags::Animations)
		    | ToCookedSceneFeatureFlagMask(CookedSceneFeatureFlags::SkinnedMeshes)
		    | ToCookedSceneFeatureFlagMask(CookedSceneFeatureFlags::MorphTargets)
		    | ToCookedSceneFeatureFlagMask(CookedSceneFeatureFlags::MaterialVariants)
		    | ToCookedSceneFeatureFlagMask(CookedSceneFeatureFlags::AuthoredMeshInstancing);
		if ((manifest.header.featureFlags & ~knownFeatureFlags) != 0u)
		{
			Invalid(std::format("Cooked scene manifest uses unknown feature flag bits 0x{:08X}", manifest.header.featureFlags));
		}

		if (!manifest.cameras.empty() && !HasFeatureFlag(manifest.header.featureFlags, CookedSceneFeatureFlags::Cameras))
		{
			Invalid("Cooked scene manifest has camera records but is missing the Cameras feature flag");
		}

		if (!manifest.lights.empty() && !HasFeatureFlag(manifest.header.featureFlags, CookedSceneFeatureFlags::Lights))
		{
			Invalid("Cooked scene manifest has light records but is missing the Lights feature flag");
		}

		if (!manifest.skeletonRefs.empty() && !HasFeatureFlag(manifest.header.featureFlags, CookedSceneFeatureFlags::Skeletons))
		{
			Invalid("Cooked scene manifest has skeleton refs but is missing the Skeletons feature flag");
		}

		if (!manifest.animationReferences.empty() && !HasFeatureFlag(manifest.header.featureFlags, CookedSceneFeatureFlags::Animations))
		{
			Invalid("Cooked scene manifest has animation refs but is missing the Animations feature flag");
		}

		if ((!manifest.materialVariants.empty() || !manifest.materialVariantMappings.empty())
		    && !HasFeatureFlag(manifest.header.featureFlags, CookedSceneFeatureFlags::MaterialVariants))
		{
			Invalid("Cooked scene manifest has material variant records but is missing the MaterialVariants feature flag");
		}
	}

	void SceneManifestValidation::ValidateMeshReferences(const LoadedSceneManifest& manifest)
	{
		for (std::size_t meshReferenceIndex = 0; meshReferenceIndex < manifest.meshAssetReferences.size(); ++meshReferenceIndex)
		{
			if (manifest.meshAssetReferences[meshReferenceIndex].meshAssetId == InvalidCookedAssetId)
			{
				Invalid(std::format("Cooked scene manifest has an invalid mesh asset reference at index {}", meshReferenceIndex));
			}
			if (manifest.meshAssetReferences[meshReferenceIndex].meshAssetKind != CookedMeshAssetKind::Static
			    && manifest.meshAssetReferences[meshReferenceIndex].meshAssetKind != CookedMeshAssetKind::Skeletal)
			{
				Invalid(std::format("Cooked scene manifest has an invalid mesh asset kind at index {}", meshReferenceIndex));
			}
		}
	}

	void SceneManifestValidation::ValidateInstanceReferenceRanges(const LoadedSceneManifest& manifest, std::size_t instanceIndex)
	{
		const CookedSceneInstanceRecord& instance = manifest.instances[instanceIndex];
		if (instance.meshAssetIndex >= manifest.meshAssetReferences.size())
		{
			Invalid(
			    std::format(
			        "Cooked scene instance {} references mesh asset index {} but only {} mesh assets exist",
			        instanceIndex,
			        instance.meshAssetIndex,
			        manifest.meshAssetReferences.size()));
		}

		if (instance.materialAssetIndex != kInvalidCookedMaterialAssetIndex
		    && instance.materialAssetIndex >= manifest.materialAssetReferences.size())
		{
			Invalid(
			    std::format(
			        "Cooked scene instance {} references material asset index {} but only {} material assets exist",
			        instanceIndex,
			        instance.materialAssetIndex,
			        manifest.materialAssetReferences.size()));
		}

		if (instance.groupIndex != kInvalidCookedSceneInstanceGroupIndex && instance.groupIndex >= manifest.instanceGroups.size())
		{
			Invalid(
			    std::format(
			        "Cooked scene instance {} references instance group index {} but only {} groups exist",
			        instanceIndex,
			        instance.groupIndex,
			        manifest.instanceGroups.size()));
		}

		if (instance.skeletonRefIndex != kInvalidCookedSceneSkeletonRefIndex && instance.skeletonRefIndex >= manifest.skeletonRefs.size())
		{
			Invalid(
			    std::format(
			        "Cooked scene instance {} references skeleton ref index {} but only {} skeleton refs exist",
			        instanceIndex,
			        instance.skeletonRefIndex,
			        manifest.skeletonRefs.size()));
		}

		if (instance.firstMorphWeight != kInvalidCookedSceneMorphWeightIndex)
		{
			if (instance.morphWeightCount == 0u || instance.firstMorphWeight >= manifest.morphWeights.size()
			    || instance.morphWeightCount > manifest.morphWeights.size() - instance.firstMorphWeight)
			{
				Invalid(
				    std::format(
				        "Cooked scene instance {} references invalid morph weight range first={} count={} with {} weights",
				        instanceIndex,
				        instance.firstMorphWeight,
				        instance.morphWeightCount,
				        manifest.morphWeights.size()));
			}
		}
		else if (instance.morphWeightCount != 0u)
		{
			Invalid(std::format("Cooked scene instance {} has morph weights without a valid first weight index", instanceIndex));
		}
	}

	void SceneManifestValidation::ValidateInstanceMeshBinding(const LoadedSceneManifest& manifest, std::size_t instanceIndex)
	{
		const CookedSceneInstanceRecord& instance = manifest.instances[instanceIndex];
		const CookedSceneMeshAssetRef& meshReference = manifest.meshAssetReferences[instance.meshAssetIndex];
		if (meshReference.meshAssetKind == CookedMeshAssetKind::Skeletal
		    && instance.skeletonRefIndex == kInvalidCookedSceneSkeletonRefIndex)
		{
			Invalid(std::format("Cooked scene instance {} uses a skeletal mesh without a skeleton ref", instanceIndex));
		}
		if (meshReference.meshAssetKind == CookedMeshAssetKind::Static && instance.skeletonRefIndex != kInvalidCookedSceneSkeletonRefIndex)
		{
			Invalid(std::format("Cooked scene instance {} uses a static mesh with a skeleton ref", instanceIndex));
		}
	}

	void SceneManifestValidation::ValidateInstances(const LoadedSceneManifest& manifest)
	{
		for (std::size_t instanceIndex = 0; instanceIndex < manifest.instances.size(); ++instanceIndex)
		{
			ValidateInstanceReferenceRanges(manifest, instanceIndex);
			ValidateInstanceMeshBinding(manifest, instanceIndex);
		}
	}

	void SceneManifestValidation::ValidateInstanceGroupReferences(const LoadedSceneManifest& manifest, std::size_t groupIndex)
	{
		const CookedSceneInstanceGroupRecord& group = manifest.instanceGroups[groupIndex];
		if (group.meshAssetIndex >= manifest.meshAssetReferences.size())
		{
			Invalid(
			    std::format(
			        "Cooked scene instance group {} references mesh asset index {} but only {} mesh assets exist",
			        groupIndex,
			        group.meshAssetIndex,
			        manifest.meshAssetReferences.size()));
		}

		if (group.materialAssetIndex != kInvalidCookedMaterialAssetIndex
		    && group.materialAssetIndex >= manifest.materialAssetReferences.size())
		{
			Invalid(
			    std::format(
			        "Cooked scene instance group {} references material asset index {} but only {} material assets exist",
			        groupIndex,
			        group.materialAssetIndex,
			        manifest.materialAssetReferences.size()));
		}
	}

	void SceneManifestValidation::ValidateInstanceGroupRange(const LoadedSceneManifest& manifest, std::size_t groupIndex)
	{
		const CookedSceneInstanceGroupRecord& group = manifest.instanceGroups[groupIndex];
		if (group.instanceCount == 0 || group.firstInstance >= manifest.instances.size()
		    || group.instanceCount > manifest.instances.size() - group.firstInstance)
		{
			Invalid(
			    std::format(
			        "Cooked scene instance group {} references invalid instance range first={} count={} with {} instances",
			        groupIndex,
			        group.firstInstance,
			        group.instanceCount,
			        manifest.instances.size()));
		}

		for (std::uint32_t instanceOffset = 0; instanceOffset < group.instanceCount; ++instanceOffset)
		{
			const std::size_t instanceIndex = static_cast<std::size_t>(group.firstInstance) + instanceOffset;
			if (manifest.instances[instanceIndex].groupIndex != groupIndex)
			{
				Invalid(
				    std::format(
				        "Cooked scene instance group {} range contains instance {} with mismatched group index {}",
				        groupIndex,
				        instanceIndex,
				        manifest.instances[instanceIndex].groupIndex));
			}
		}
	}

	void SceneManifestValidation::ValidateInstanceGroupKind(const LoadedSceneManifest& manifest, std::size_t groupIndex)
	{
		const CookedSceneInstanceGroupRecord& group = manifest.instanceGroups[groupIndex];
		if (group.groupKind != CookedSceneInstanceGroupKind::None && group.groupKind != CookedSceneInstanceGroupKind::SharedMeshReference
		    && group.groupKind != CookedSceneInstanceGroupKind::AuthoredInstanceGroup)
		{
			Invalid(std::format("Cooked scene instance group {} uses an unknown group kind", groupIndex));
		}
	}

	void SceneManifestValidation::ValidateInstanceGroups(const LoadedSceneManifest& manifest)
	{
		for (std::size_t groupIndex = 0; groupIndex < manifest.instanceGroups.size(); ++groupIndex)
		{
			ValidateInstanceGroupReferences(manifest, groupIndex);
			ValidateInstanceGroupRange(manifest, groupIndex);
			ValidateInstanceGroupKind(manifest, groupIndex);
		}
	}

	void SceneManifestValidation::ValidateCameras(const LoadedSceneManifest& manifest)
	{
		for (std::size_t cameraIndex = 0; cameraIndex < manifest.cameras.size(); ++cameraIndex)
		{
			const CookedSceneCameraRecord& camera = manifest.cameras[cameraIndex];
			if (!HasTerminatedName(camera.name) || camera.projectionKind != CookedSceneCameraProjectionKind::Perspective
			    || camera.sourceNodeIndex == (std::numeric_limits<std::uint32_t>::max)() || camera.flags != 0u)
			{
				Invalid(std::format("Cooked scene camera {} has an unsupported or invalid projection", cameraIndex));
			}
		}
	}

	void SceneManifestValidation::ValidateLights(const LoadedSceneManifest& manifest)
	{
		for (std::size_t lightIndex = 0; lightIndex < manifest.lights.size(); ++lightIndex)
		{
			const CookedSceneLightRecord& light = manifest.lights[lightIndex];
			if (!HasTerminatedName(light.name)
			    || (light.kind != CookedSceneLightKind::Directional && light.kind != CookedSceneLightKind::Point
			        && light.kind != CookedSceneLightKind::Spot && light.kind != CookedSceneLightKind::Rect))
			{
				Invalid(std::format("Cooked scene light {} uses an unknown light kind", lightIndex));
			}

			if (light.sourceNodeIndex == kInvalidCookedSceneLightSourceNodeIndex || (light.flags & ~1u) != 0u)
			{
				Invalid(std::format("Cooked scene light {} has invalid structural metadata", lightIndex));
			}
		}
	}

	void SceneManifestValidation::ValidateSkeletonRefs(const LoadedSceneManifest& manifest)
	{
		for (std::size_t skeletonIndex = 0; skeletonIndex < manifest.skeletonRefs.size(); ++skeletonIndex)
		{
			if (manifest.skeletonRefs[skeletonIndex].skeletonAssetId == InvalidCookedAssetId)
			{
				Invalid(std::format("Cooked scene skeleton ref {} has an invalid asset id", skeletonIndex));
			}
		}
	}

	void SceneManifestValidation::ValidateAnimationRefs(const LoadedSceneManifest& manifest)
	{
		for (std::size_t animationIndex = 0; animationIndex < manifest.animationReferences.size(); ++animationIndex)
		{
			if (manifest.animationReferences[animationIndex].animationAssetId == InvalidCookedAssetId)
			{
				Invalid(std::format("Cooked scene animation ref {} has an invalid asset id", animationIndex));
			}
		}
	}

	void SceneManifestValidation::ValidateMetadata(const LoadedSceneManifest& manifest)
	{
		ValidateCameras(manifest);
		ValidateLights(manifest);
		ValidateSkeletonRefs(manifest);
		ValidateAnimationRefs(manifest);
	}

	void SceneManifestValidation::ValidateMaterialVariants(const LoadedSceneManifest& manifest)
	{
		for (std::size_t variantIndex = 0; variantIndex < manifest.materialVariants.size(); ++variantIndex)
		{
			if (!HasValidName(manifest.materialVariants[variantIndex].name))
			{
				Invalid(std::format("Cooked scene material variant {} has an invalid name", variantIndex));
			}
		}

		for (std::size_t mappingIndex = 0; mappingIndex < manifest.materialVariantMappings.size(); ++mappingIndex)
		{
			const CookedSceneMaterialVariantMappingRecord& mapping = manifest.materialVariantMappings[mappingIndex];
			if (mapping.meshAssetIndex >= manifest.meshAssetReferences.size())
			{
				Invalid(std::format("Cooked scene material variant mapping {} references an invalid mesh asset", mappingIndex));
			}

			if (mapping.variantIndex >= manifest.materialVariants.size())
			{
				Invalid(std::format("Cooked scene material variant mapping {} references an invalid variant", mappingIndex));
			}

			if (mapping.materialAssetIndex >= manifest.materialAssetReferences.size())
			{
				Invalid(std::format("Cooked scene material variant mapping {} references an invalid material", mappingIndex));
			}
		}
	}

	void SceneManifestValidator::ValidateHeader(const LoadedSceneManifest& manifest)
	{
		if (manifest.header.fileHeader.magic != kCookedSceneManifestMagic)
		{
			SceneManifestValidation::Invalid("Invalid cooked scene manifest magic");
		}

		if (manifest.header.fileHeader.version != kCookedSceneManifestVersion)
		{
			SceneManifestValidation::Invalid(
			    std::format(
			        "Cooked scene manifest version {} is not supported by this runtime; expected version {}. Recook the scene asset.",
			        manifest.header.fileHeader.version,
			        kCookedSceneManifestVersion));
		}

		if (manifest.header.coordinateContractVersion != WorldCoordinates::kCoordinateContractVersion)
		{
			SceneManifestValidation::Invalid(
			    std::format(
			        "Cooked scene coordinate contract version {} is not supported; expected {}. Recook the scene asset.",
			        manifest.header.coordinateContractVersion,
			        WorldCoordinates::kCoordinateContractVersion));
		}
	}

	void SceneManifestValidator::ValidateRecords(const LoadedSceneManifest& manifest)
	{
		SceneManifestValidation::ValidateFeatures(manifest);
		SceneManifestValidation::ValidateMeshReferences(manifest);
		SceneManifestValidation::ValidateInstances(manifest);
		SceneManifestValidation::ValidateInstanceGroups(manifest);
		SceneManifestValidation::ValidateMetadata(manifest);
		SceneManifestValidation::ValidateMaterialVariants(manifest);
	}
}
