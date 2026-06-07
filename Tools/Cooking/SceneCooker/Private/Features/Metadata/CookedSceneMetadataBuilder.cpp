#include "PCH.h"

#include "Features/Metadata/CookedSceneMetadataBuilder.h"

#include <cstdint>

namespace
{
	void AddFeatureFlag(std::uint32_t& flags, Assets::CookedSceneFeatureFlags flag) noexcept
	{
		flags |= Assets::ToCookedSceneFeatureFlagMask(flag);
	}

	std::uint32_t BuildFeatureFlags(const SourceImportResult& importResult) noexcept
	{
		std::uint32_t flags = 0;
		if (!importResult.scene.cameras.empty())
		{
			AddFeatureFlag(flags, Assets::CookedSceneFeatureFlags::Cameras);
		}

		if (!importResult.scene.lights.empty())
		{
			AddFeatureFlag(flags, Assets::CookedSceneFeatureFlags::Lights);
		}

		if (importResult.diagnostics.sceneFeatures.skinnedNodeCount > 0)
		{
			AddFeatureFlag(flags, Assets::CookedSceneFeatureFlags::SkinnedMeshes);
			AddFeatureFlag(flags, Assets::CookedSceneFeatureFlags::Skeletons);
		}

		if (importResult.diagnostics.sceneFeatures.animationCount > 0)
		{
			AddFeatureFlag(flags, Assets::CookedSceneFeatureFlags::Animations);
		}

		bool hasSkeletalMorphTargets = false;
		for (const ImportedMeshPrimitive& primitive : importResult.scene.meshPrimitives)
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

		if (importResult.diagnostics.sceneFeatures.materialVariantCount > 0 ||
		    importResult.diagnostics.sceneFeatures.materialVariantPrimitiveCount > 0)
		{
			AddFeatureFlag(flags, Assets::CookedSceneFeatureFlags::MaterialVariants);
		}

		if (importResult.diagnostics.sceneFeatures.authoredInstancingNodeCount > 0)
		{
			AddFeatureFlag(flags, Assets::CookedSceneFeatureFlags::AuthoredMeshInstancing);
		}

		return flags;
	}
}  // namespace

void CookedSceneMetadataBuilder::BuildMetadata(const SourceImportResult& importResult, CookedSceneBuild& outBuild)
{
	outBuild.manifest.header.featureFlags = BuildFeatureFlags(importResult);
}
