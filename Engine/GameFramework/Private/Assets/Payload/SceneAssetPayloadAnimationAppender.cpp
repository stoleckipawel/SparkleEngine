#include "PCH.h"

#include "Assets/Payload/SceneAssetPayloadAnimationAppender.h"

#include "Assets/Cooked/LoadedAnimationAsset.h"
#include "Assets/Loaders/AnimationAssetLoader.h"
#include "Assets/Loaders/CookedAssetFileSet.h"
#include "Assets/Translators/SceneAssetAnimationTranslator.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Paths/DirectoryPaths.h"

#include <filesystem>
#include <format>

namespace Assets
{
	bool SceneAssetPayloadAnimationAppender::AppendAnimations(
	    const LoadedSceneManifest& sceneManifest,
	    const CookedAssetFileSet& files,
	    SceneAssetPayload& sceneAssetPayload,
	    std::string& errorMessage)
	{
		AnimationAssetLoader animationAssetLoader;
		sceneAssetPayload.animations.reserve(sceneAssetPayload.animations.size() + sceneManifest.animationRefs.size());

		for (const CookedSceneAnimationRef& animationRef : sceneManifest.animationRefs)
		{
			LoadedAnimationAsset animationAsset;
			const std::filesystem::path animationAssetPath = Paths::CookedAnimationAsset(animationRef.animationAssetId);
			if (!animationAssetLoader.Decode(animationAssetPath, files.Find(animationAssetPath), animationAsset, errorMessage))
			{
				errorMessage = std::format(
				    "Failed to load cooked animation asset {} from '{}' - {}",
				    Formatting::FormatHexUInt64(animationRef.animationAssetId),
				    animationAssetPath.string(),
				    errorMessage);
				return false;
			}

			sceneAssetPayload.animations.push_back(BuildSceneAssetAnimation(animationAsset, animationRef.animationAssetId));
		}

		return true;
	}
}
