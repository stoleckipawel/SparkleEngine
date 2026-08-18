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
	void SceneAssetPayloadAnimationAppender::AppendAnimations(
	    const LoadedSceneManifest& sceneManifest,
	    CookedAssetFileSet& files,
	    SceneAssetPayload& sceneAssetPayload)
	{
		AnimationAssetLoader animationAssetLoader;
		sceneAssetPayload.animations.reserve(sceneAssetPayload.animations.size() + sceneManifest.animationReferences.size());

		for (const CookedAnimationReference& animationRef : sceneManifest.animationReferences)
		{
			const std::filesystem::path animationAssetPath = Paths::CookedAnimationAsset(animationRef.animationAssetId);
			const LoadedAnimationAsset animationAsset = animationAssetLoader.Decode(animationAssetPath, files.Get(animationAssetPath));
			sceneAssetPayload.animations.push_back(BuildSceneAssetAnimation(animationAsset, animationRef.animationAssetId));
			files.Release(animationAssetPath);
		}
	}
}
