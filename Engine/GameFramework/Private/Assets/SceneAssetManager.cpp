#include "PCH.h"

#include "Assets/SceneAssetManager.h"

#include "Assets/Cooked/LoadedMaterialAsset.h"
#include "Assets/Cooked/LoadedSceneManifest.h"
#include "Assets/CookedAssembly/CookedMaterialTranslator.h"
#include "Assets/SceneAssetRegistry.h"
#include "Assets/Loaders/MaterialAssetLoader.h"
#include "Assets/Loaders/MeshAssetLoader.h"
#include "Assets/Loaders/SceneManifestLoader.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Paths/DirectoryPaths.h"

#include <format>

static const auto g_sceneAssetManagerLogger = Logging::GetOrCreateLogger("GameFramework.SceneAssets");

namespace Assets
{
	SceneAssetLoadResult SceneAssetManager::LoadSceneAsset(const SceneAssetId& sceneAssetId)
	{
		SceneAssetLoadResult result;
		if (!EnsureRegistryLoaded(result.errorMessage))
		{
			return result;
		}

		std::uint32_t materialBaseIndex = 0;
		if (AppendSceneAssetToPayload(sceneAssetId, result.sceneAssetPayload, materialBaseIndex, result.errorMessage))
		{
			m_loadedSceneAssetIds.push_back(sceneAssetId.value);
		}
		return result;
	}

	SceneAssetLoadResult SceneAssetManager::LoadSceneAssets(std::span<const SceneAssetId> sceneAssetIds)
	{
		SceneAssetLoadResult result;
		if (!EnsureRegistryLoaded(result.errorMessage))
		{
			return result;
		}

		std::uint32_t materialBaseIndex = 0;

		for (const SceneAssetId& sceneAssetId : sceneAssetIds)
		{
			if (!AppendSceneAssetToPayload(sceneAssetId, result.sceneAssetPayload, materialBaseIndex, result.errorMessage))
			{
				return result;
			}

			m_loadedSceneAssetIds.push_back(sceneAssetId.value);
		}

		return result;
	}

	void SceneAssetManager::UnloadAll() noexcept
	{
		m_loadedSceneAssetIds.clear();
	}

	bool SceneAssetManager::EnsureRegistryLoaded(std::string& errorMessage)
	{
		if (m_sceneAssetRegistryLoaded)
		{
			errorMessage.clear();
			return true;
		}

		if (!m_sceneAssetRegistry.Load(errorMessage))
		{
			errorMessage =
			    std::format("Failed to load scene asset registry from '{}' - {}", Paths::SceneAssetRegistry().string(), errorMessage);
			return false;
		}

		m_sceneAssetRegistryLoaded = true;
		errorMessage.clear();
		return true;
	}

	bool SceneAssetManager::AppendSceneAssetToPayload(
	    const SceneAssetId& sceneAssetId,
	    SceneAssetPayload& sceneAssetPayload,
	    std::uint32_t& materialBaseIndex,
	    std::string& errorMessage)
	{
		if (sceneAssetId.IsEmpty())
		{
			return true;
		}

		SceneManifestLoader sceneManifestLoader;
		LoadedSceneManifest sceneManifest;
		const auto manifestRelativePath = m_sceneAssetRegistry.Resolve(sceneAssetId.value);
		if (!manifestRelativePath)
		{
			errorMessage = std::format("Scene asset id '{}' is not registered in the cooked scene asset registry", sceneAssetId.value);
			return false;
		}
		const std::filesystem::path sceneManifestPath = Paths::CookedSceneManifestRelative(*manifestRelativePath);

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
		struct LoadedMeshAsset final
		{
			MeshData mesh;
			CookedAssetId assetId = InvalidCookedAssetId;
		};
		std::vector<LoadedMeshAsset> loadedMeshes;
		loadedMeshes.reserve(sceneManifest.meshAssetReferences.size());
		for (const CookedSceneMeshAssetRef& meshReference : sceneManifest.meshAssetReferences)
		{
			LoadedMeshAsset loadedMesh;
			const std::filesystem::path meshAssetPath = Paths::CookedMeshAsset(meshReference.meshAssetId);
			if (!meshAssetLoader.Load(meshAssetPath, loadedMesh.mesh, errorMessage))
			{
				errorMessage = std::format(
				    "Failed to load cooked mesh asset {} from '{}' - {}",
				    Formatting::FormatHexUInt64(meshReference.meshAssetId),
				    meshAssetPath.string(),
				    errorMessage);
				return false;
			}

			loadedMesh.assetId = meshReference.meshAssetId;
			loadedMeshes.push_back(std::move(loadedMesh));
		}

		MaterialAssetLoader materialAssetLoader;
		CookedMaterialTranslator materialTranslator;
		sceneAssetPayload.materials.reserve(sceneAssetPayload.materials.size() + sceneManifest.materialAssetReferences.size());
		for (const CookedSceneMaterialAssetRef& materialReference : sceneManifest.materialAssetReferences)
		{
			LoadedMaterialAsset materialAsset;
			const std::filesystem::path materialAssetPath = Paths::CookedMaterialAsset(materialReference.materialAssetId);
			if (!materialAssetLoader.Load(materialAssetPath, materialAsset, errorMessage))
			{
				errorMessage = std::format(
				    "Failed to load cooked material asset {} from '{}' - {}",
				    Formatting::FormatHexUInt64(materialReference.materialAssetId),
				    materialAssetPath.string(),
				    errorMessage);
				return false;
			}

			MaterialDesc runtimeMaterial;
			if (!materialTranslator.Translate(materialAsset, runtimeMaterial, errorMessage))
			{
				errorMessage = std::format(
				    "Failed to translate cooked material asset {} from '{}' - {}",
				    Formatting::FormatHexUInt64(materialReference.materialAssetId),
				    materialAssetPath.string(),
				    errorMessage);
				return false;
			}

			sceneAssetPayload.materials.push_back(std::move(runtimeMaterial));
		}

		sceneAssetPayload.meshInstances.reserve(sceneAssetPayload.meshInstances.size() + sceneManifest.instances.size());

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

			const LoadedMeshAsset& loadedMesh = loadedMeshes[instanceRecord.meshAssetIndex];
			SceneAssetPayload::MeshInstance meshInstance;
			meshInstance.mesh = loadedMesh.mesh;
			meshInstance.assetId = loadedMesh.assetId;
			meshInstance.transform = Transform(DirectX::XMLoadFloat4x4(&instanceRecord.worldTransform));
			meshInstance.material = instanceRecord.materialAssetIndex == kInvalidCookedMaterialAssetIndex
			                            ? MaterialHandle::Invalid()
			                            : MaterialHandle(materialBaseIndex + instanceRecord.materialAssetIndex);
			sceneAssetPayload.meshInstances.push_back(std::move(meshInstance));
		}

		sceneAssetPayload.diagnostics.loadedSceneAssetCount += 1u;
		sceneAssetPayload.diagnostics.meshAssetReferenceCount += sceneManifest.meshAssetReferences.size();
		sceneAssetPayload.diagnostics.meshInstanceCount += sceneManifest.instances.size();
		sceneAssetPayload.diagnostics.meshInstanceGroupCount += sceneManifest.instanceGroups.size();

		SPDLOG_LOGGER_INFO(
		    g_sceneAssetManagerLogger,
		    "SceneAssetManager: Loaded scene asset '{}' - meshAssetRefs={}, meshInstances={}, instanceGroups={}, materials={}",
		    sceneAssetId.value,
		    sceneManifest.meshAssetReferences.size(),
		    sceneManifest.instances.size(),
		    sceneManifest.instanceGroups.size(),
		    sceneManifest.materialAssetReferences.size());

		materialBaseIndex += static_cast<std::uint32_t>(sceneManifest.materialAssetReferences.size());
		errorMessage.clear();
		return true;
	}

}