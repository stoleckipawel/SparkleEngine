#include "PCH.h"

#include "Assets/Payload/SceneAssetPayloadSkeletonAppender.h"

#include "Assets/Cooked/LoadedSkeletonAsset.h"
#include "Assets/Loaders/SkeletonAssetLoader.h"
#include "Assets/Loaders/CookedAssetFileSet.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Assets/Translators/SceneAssetSkeletonTranslator.h"

#include <filesystem>
#include <format>

namespace Assets
{
	void SceneAssetPayloadSkeletonAppender::AppendSkeletons(
	    const LoadedSceneManifest& sceneManifest,
	    CookedAssetFileSet& files,
	    SceneAssetPayload& sceneAssetPayload)
	{
		SkeletonAssetLoader skeletonAssetLoader;
		sceneAssetPayload.skeletons.reserve(sceneAssetPayload.skeletons.size() + sceneManifest.skeletonRefs.size());

		for (const CookedSceneSkeletonRef& skeletonRef : sceneManifest.skeletonRefs)
		{
			const std::filesystem::path skeletonAssetPath = Paths::CookedSkeletonAsset(skeletonRef.skeletonAssetId);
			const LoadedSkeletonAsset skeletonAsset = skeletonAssetLoader.Decode(skeletonAssetPath, files.Get(skeletonAssetPath));
			sceneAssetPayload.skeletons.push_back(
			    BuildSceneAssetSkeleton(skeletonAsset, skeletonRef.skeletonAssetId, skeletonRef.sourceSkinIndex));
			files.Release(skeletonAssetPath);
		}
	}
}
