#include "PCH.h"

#include "Assets/Loaders/SceneManifestFeatureValidator.h"

#include "Assets/Cooked/LoadedSceneManifest.h"

#include <format>

namespace Assets::SceneManifestFeatureValidator
{
	namespace
	{
		bool HasFeatureFlag(std::uint32_t flags, CookedSceneFeatureFlags feature) noexcept
		{
			return (flags & ToCookedSceneFeatureFlagMask(feature)) != 0u;
		}
	}  // namespace

	bool Validate(const LoadedSceneManifest& manifest, std::string& outErrorMessage)
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

		return true;
	}
}
