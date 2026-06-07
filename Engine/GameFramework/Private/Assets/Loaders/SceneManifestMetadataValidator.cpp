#include "PCH.h"

#include "Assets/Loaders/SceneManifestMetadataValidator.h"

#include "Assets/Cooked/LoadedSceneManifest.h"

#include <cstddef>
#include <format>

namespace Assets::SceneManifestMetadataValidator
{
	namespace
	{
		bool ValidateCameras(const LoadedSceneManifest& manifest, std::string& outErrorMessage)
		{
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

			return true;
		}

		bool ValidateLights(const LoadedSceneManifest& manifest, std::string& outErrorMessage)
		{
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

			return true;
		}

		bool ValidateSkeletonRefs(const LoadedSceneManifest& manifest, std::string& outErrorMessage)
		{
			for (std::size_t skeletonIndex = 0; skeletonIndex < manifest.skeletonRefs.size(); ++skeletonIndex)
			{
				if (manifest.skeletonRefs[skeletonIndex].skeletonAssetId == InvalidCookedAssetId)
				{
					outErrorMessage = std::format("Cooked scene skeleton ref {} has an invalid asset id", skeletonIndex);
					return false;
				}
			}

			return true;
		}

		bool ValidateAnimationRefs(const LoadedSceneManifest& manifest, std::string& outErrorMessage)
		{
			for (std::size_t animationIndex = 0; animationIndex < manifest.animationRefs.size(); ++animationIndex)
			{
				if (manifest.animationRefs[animationIndex].animationAssetId == InvalidCookedAssetId)
				{
					outErrorMessage = std::format("Cooked scene animation ref {} has an invalid asset id", animationIndex);
					return false;
				}
			}

			return true;
		}
	}  // namespace

	bool Validate(const LoadedSceneManifest& manifest, std::string& outErrorMessage)
	{
		return ValidateCameras(manifest, outErrorMessage) &&
		       ValidateLights(manifest, outErrorMessage) &&
		       ValidateSkeletonRefs(manifest, outErrorMessage) &&
		       ValidateAnimationRefs(manifest, outErrorMessage);
	}
}
