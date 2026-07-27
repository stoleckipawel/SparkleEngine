#include "PCH.h"

#include "Assets/Payload/SceneAssetPayloadMeshBindings.h"

namespace Assets
{
	std::vector<SceneAssetPayloadMeshBinding> BuildSceneAssetPayloadMeshBindings(const LoadedSceneManifest& sceneManifest)
	{
		std::vector<SceneAssetPayloadMeshBinding> bindings;
		bindings.reserve(sceneManifest.meshAssetReferences.size());

		SceneMeshAssetIndex staticMeshIndex = 0;
		SceneMeshAssetIndex skeletalMeshIndex = 0;
		for (const CookedSceneMeshAssetRef& meshReference : sceneManifest.meshAssetReferences)
		{
			SceneAssetPayloadMeshBinding& binding = bindings.emplace_back();
			binding.kind = meshReference.meshAssetKind;
			binding.payloadMeshAssetIndex =
			    meshReference.meshAssetKind == CookedMeshAssetKind::Skeletal ? skeletalMeshIndex++ : staticMeshIndex++;
		}

		return bindings;
	}
}
