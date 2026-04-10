#include "PCH.h"

#include "Assets/SceneAssetManager.h"

#include "Assets/MaterialAssetTranslator.h"
#include "Assets/Loaders/LoadedCookedAssets.h"
#include "Assets/Loaders/MaterialAssetLoader.h"
#include "Assets/Loaders/MeshAssetLoader.h"
#include "Assets/Loaders/SceneManifestLoader.h"
#include "Core/Public/FileSystemUtils.h"

#include <format>

namespace Engine::Assets
{
	SceneAssetLoadResult SceneAssetManager::LoadSceneAsset(const SceneAssetId& sceneAssetId)
	{
		SceneAssetLoadResult result;
		std::uint32_t materialBaseIndex = 0;
		AppendSceneAssetToPayload(sceneAssetId, result.payload, materialBaseIndex, result.errorMessage);
		return result;
	}

	SceneAssetLoadResult SceneAssetManager::LoadSceneAssets(std::span<const SceneAssetId> sceneAssetIds)
	{
		SceneAssetLoadResult result;
		std::uint32_t materialBaseIndex = 0;

		for (const SceneAssetId& sceneAssetId : sceneAssetIds)
		{
			if (!AppendSceneAssetToPayload(sceneAssetId, result.payload, materialBaseIndex, result.errorMessage))
			{
				return result;
			}
		}

		return result;
	}

	void SceneAssetManager::UnloadAll() noexcept {}

	bool SceneAssetManager::AppendSceneAssetToPayload(
	    const SceneAssetId& sceneAssetId,
	    RuntimeScenePayload& payload,
	    std::uint32_t& materialBaseIndex,
	    std::string& errorMessage)
	{
		if (sceneAssetId.IsEmpty())
		{
			return true;
		}

		SceneManifestLoader sceneManifestLoader;
		LoadedSceneManifest sceneManifest;
		const std::filesystem::path sceneManifestPath = BuildSceneManifestPath(sceneAssetId);
		if (!sceneManifestLoader.Load(sceneManifestPath, sceneManifest, errorMessage))
		{
			errorMessage = std::format(
			    "Failed to load cooked scene manifest for '{}' from '{}' - {}",
			    sceneAssetId.value,
			    sceneManifestPath.string(),
			    errorMessage);
			return false;
		}

		MeshAssetLoader meshAssetLoader;
		std::vector<MeshData> loadedMeshes;
		loadedMeshes.reserve(sceneManifest.meshAssetReferences.size());
		for (const CookedSceneMeshAssetRef& meshReference : sceneManifest.meshAssetReferences)
		{
			MeshData meshData;
			const std::filesystem::path meshAssetPath = BuildMeshAssetPath(meshReference.meshAssetId);
			if (!meshAssetLoader.Load(meshAssetPath, meshData, errorMessage))
			{
				errorMessage = std::format(
				    "Failed to load cooked mesh asset {:016X} from '{}' - {}",
				    meshReference.meshAssetId,
				    meshAssetPath.string(),
				    errorMessage);
				return false;
			}

			loadedMeshes.push_back(std::move(meshData));
		}

		MaterialAssetLoader materialAssetLoader;
		MaterialAssetTranslator materialAssetTranslator;
		payload.materials.reserve(payload.materials.size() + sceneManifest.materialAssetReferences.size());
		for (const CookedSceneMaterialAssetRef& materialReference : sceneManifest.materialAssetReferences)
		{
			LoadedMaterialAsset materialAsset;
			const std::filesystem::path materialAssetPath = BuildMaterialAssetPath(materialReference.materialAssetId);
			if (!materialAssetLoader.Load(materialAssetPath, materialAsset, errorMessage))
			{
				errorMessage = std::format(
				    "Failed to load cooked material asset {:016X} from '{}' - {}",
				    materialReference.materialAssetId,
				    materialAssetPath.string(),
				    errorMessage);
				return false;
			}

			MaterialDesc runtimeMaterial;
			if (!materialAssetTranslator.Translate(materialAsset, runtimeMaterial, errorMessage))
			{
				errorMessage = std::format(
				    "Failed to translate cooked material asset {:016X} from '{}' - {}",
				    materialReference.materialAssetId,
				    materialAssetPath.string(),
				    errorMessage);
				return false;
			}

			payload.materials.push_back(std::move(runtimeMaterial));
		}

		payload.meshes.reserve(payload.meshes.size() + sceneManifest.instances.size());
		payload.transforms.reserve(payload.transforms.size() + sceneManifest.instances.size());
		payload.materialHandles.reserve(payload.materialHandles.size() + sceneManifest.instances.size());

		for (const CookedSceneInstanceRecord& instanceRecord : sceneManifest.instances)
		{
			if (instanceRecord.meshAssetIndex >= loadedMeshes.size())
			{
				errorMessage = std::format(
				    "Scene manifest '{}' references mesh index {} but only {} mesh assets were loaded",
				    sceneAssetId.value,
				    instanceRecord.meshAssetIndex,
				    loadedMeshes.size());
				return false;
			}

			if (instanceRecord.materialAssetIndex != kInvalidCookedMaterialAssetIndex &&
			    instanceRecord.materialAssetIndex >= sceneManifest.materialAssetReferences.size())
			{
				errorMessage = std::format(
				    "Scene manifest '{}' references material index {} but only {} material assets were loaded",
				    sceneAssetId.value,
				    instanceRecord.materialAssetIndex,
				    sceneManifest.materialAssetReferences.size());
				return false;
			}

			payload.meshes.push_back(loadedMeshes[instanceRecord.meshAssetIndex]);
			payload.transforms.emplace_back(DirectX::XMLoadFloat4x4(&instanceRecord.worldTransform));
			payload.materialHandles.emplace_back(
			    instanceRecord.materialAssetIndex == kInvalidCookedMaterialAssetIndex
			        ? MaterialHandle::Invalid()
			        : MaterialHandle(materialBaseIndex + instanceRecord.materialAssetIndex));
		}

		materialBaseIndex += static_cast<std::uint32_t>(sceneManifest.materialAssetReferences.size());
		errorMessage.clear();
		return true;
	}

	std::filesystem::path SceneAssetManager::GetCookedAssetRootPath()
	{
		return Filesystem::GetProjectAssetsPath() / "Cooked";
	}

	std::filesystem::path SceneAssetManager::BuildSceneManifestPath(const SceneAssetId& sceneAssetId)
	{
		std::filesystem::path relativeScenePath(sceneAssetId.value);
		relativeScenePath.replace_extension(".sscn");
		return GetCookedAssetRootPath() / "SceneManifests" / relativeScenePath;
	}

	std::filesystem::path SceneAssetManager::BuildMeshAssetPath(CookedAssetId meshAssetId)
	{
		return GetCookedAssetRootPath() / "Meshes" / std::format("{:016X}.smsh", meshAssetId);
	}

	std::filesystem::path SceneAssetManager::BuildMaterialAssetPath(CookedAssetId materialAssetId)
	{
		return GetCookedAssetRootPath() / "Materials" / std::format("{:016X}.smat", materialAssetId);
	}
}