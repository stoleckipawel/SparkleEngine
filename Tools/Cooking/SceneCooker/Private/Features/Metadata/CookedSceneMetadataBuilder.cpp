#include "PCH.h"

#include "Features/Metadata/CookedSceneMetadataBuilder.h"

#include <cstdint>

class CookedSceneFeatureMetadata final
{
  public:
	static void AddFeatureFlag(std::uint32_t& flags, Assets::CookedSceneFeatureFlags flag) noexcept
	{
		flags |= Assets::ToCookedSceneFeatureFlagMask(flag);
	}

	static std::uint32_t BuildFeatureFlags(const SourceImportOutput& importOutput) noexcept
	{
		std::uint32_t flags = 0;
		if (!importOutput.scene.cameras.empty())
		{
			AddFeatureFlag(flags, Assets::CookedSceneFeatureFlags::Cameras);
		}

		if (!importOutput.scene.lights.empty())
		{
			AddFeatureFlag(flags, Assets::CookedSceneFeatureFlags::Lights);
		}

		bool hasSkinnedMeshes = !importOutput.scene.skeletons.empty();
		for (const ImportedMeshPrimitive& primitive : importOutput.scene.meshPrimitives)
		{
			if (primitive.geometry.HasSkinInfluences())
			{
				hasSkinnedMeshes = true;
				break;
			}
		}

		if (hasSkinnedMeshes)
		{
			AddFeatureFlag(flags, Assets::CookedSceneFeatureFlags::SkinnedMeshes);
			AddFeatureFlag(flags, Assets::CookedSceneFeatureFlags::Skeletons);
		}

		if (!importOutput.scene.animations.empty())
		{
			AddFeatureFlag(flags, Assets::CookedSceneFeatureFlags::Animations);
		}

		bool hasSkeletalMorphTargets = false;
		for (const ImportedMeshPrimitive& primitive : importOutput.scene.meshPrimitives)
		{
			if (primitive.geometry.HasSkinInfluences() && primitive.geometry.HasMorphTargets())
			{
				hasSkeletalMorphTargets = true;
				break;
			}
		}

		if (hasSkeletalMorphTargets)
		{
			AddFeatureFlag(flags, Assets::CookedSceneFeatureFlags::MorphTargets);
		}

		if (!importOutput.scene.materialVariants.empty() || !importOutput.scene.materialVariantMappings.empty())
		{
			AddFeatureFlag(flags, Assets::CookedSceneFeatureFlags::MaterialVariants);
		}

		for (const ImportedMeshInstanceGroup& group : importOutput.scene.meshInstanceGroups)
		{
			if (group.groupKind == ImportedMeshInstanceGroupKind::AuthoredInstanceGroup)
			{
				AddFeatureFlag(flags, Assets::CookedSceneFeatureFlags::AuthoredMeshInstancing);
				break;
			}
		}

		return flags;
	}
};

void CookedSceneMetadataBuilder::BuildMetadata(const SourceImportOutput& importOutput, CookedSceneBuild& outBuild)
{
	outBuild.manifest.header.featureFlags = CookedSceneFeatureMetadata::BuildFeatureFlags(importOutput);
}
