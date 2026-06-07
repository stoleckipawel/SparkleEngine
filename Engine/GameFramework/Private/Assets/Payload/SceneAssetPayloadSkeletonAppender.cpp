#include "PCH.h"

#include "Assets/Payload/SceneAssetPayloadSkeletonAppender.h"

#include "Assets/Cooked/LoadedSkeletonAsset.h"
#include "Assets/Loaders/SkeletonAssetLoader.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Assets/Translators/SceneAssetSkeletonTranslator.h"

#include <filesystem>
#include <format>

namespace Assets
{
	bool SceneAssetPayloadSkeletonAppender::AppendSkeletons(
	    const LoadedSceneManifest& sceneManifest,
	    SceneAssetPayload& sceneAssetPayload,
	    std::string& errorMessage)
	{
		SkeletonAssetLoader skeletonAssetLoader;
		sceneAssetPayload.skeletons.reserve(sceneAssetPayload.skeletons.size() + sceneManifest.skeletonRefs.size());

		for (const CookedSceneSkeletonRef& skeletonRef : sceneManifest.skeletonRefs)
		{
			LoadedSkeletonAsset skeletonAsset;
			const std::filesystem::path skeletonAssetPath = Paths::CookedSkeletonAsset(skeletonRef.skeletonAssetId);
			if (!skeletonAssetLoader.Load(skeletonAssetPath, skeletonAsset, errorMessage))
			{
				errorMessage = std::format(
				    "Failed to load cooked skeleton asset {} from '{}' - {}",
				    Formatting::FormatHexUInt64(skeletonRef.skeletonAssetId),
				    skeletonAssetPath.string(),
				    errorMessage);
				return false;
			}

			sceneAssetPayload.skeletons.push_back(BuildSceneAssetSkeleton(skeletonAsset, skeletonRef.skeletonAssetId, skeletonRef.sourceSkinIndex));
		}

		return true;
	}
}
