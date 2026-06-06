#include "PCH.h"

#include "Assets/SceneAssetManager.h"

#include "Assets/Cooked/LoadedMaterialAsset.h"
#include "Assets/Cooked/LoadedSceneManifest.h"
#include "Assets/CookedAssembly/CookedMaterialTranslator.h"
#include "Assets/SceneAssetRegistry.h"
#include "Assets/Loaders/MaterialAssetLoader.h"
#include "Assets/Loaders/MeshAssetLoader.h"
#include "Assets/Loaders/SceneManifestLoader.h"
#include "Translators/SceneAssetCameraTranslator.h"
#include "Translators/SceneAssetLightTranslator.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Scene/Transform.h"

#include <DirectXMath.h>

#include <cmath>
#include <format>

static const auto g_sceneAssetManagerLogger = Logging::GetOrCreateLogger("GameFramework.SceneAssets");

namespace Assets
{
	static SceneMeshInstanceGroupKind ToSceneMeshInstanceGroupKind(CookedSceneInstanceGroupKind groupKind) noexcept
	{
		switch (groupKind)
		{
			case CookedSceneInstanceGroupKind::SharedMeshReference:
				return SceneMeshInstanceGroupKind::SharedMeshReference;
			case CookedSceneInstanceGroupKind::AuthoredInstanceGroup:
				return SceneMeshInstanceGroupKind::AuthoredInstanceGroup;
			case CookedSceneInstanceGroupKind::None:
			default:
				return SceneMeshInstanceGroupKind::None;
		}
	}

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
		const auto meshAssetBaseIndex = static_cast<SceneMeshAssetIndex>(sceneAssetPayload.meshAssets.size());
		sceneAssetPayload.meshAssets.reserve(sceneAssetPayload.meshAssets.size() + sceneManifest.meshAssetReferences.size());
		for (const CookedSceneMeshAssetRef& meshReference : sceneManifest.meshAssetReferences)
		{
			SceneAssetPayload::MeshAsset loadedMesh;
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
			sceneAssetPayload.meshAssets.push_back(std::move(loadedMesh));
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

		const auto instanceBaseIndex = static_cast<SceneMeshInstanceIndex>(sceneAssetPayload.meshInstances.size());
		const auto groupBaseIndex = static_cast<SceneMeshInstanceGroupIndex>(sceneAssetPayload.meshInstanceGroups.size());
		sceneAssetPayload.meshInstances.reserve(sceneAssetPayload.meshInstances.size() + sceneManifest.instances.size());
		sceneAssetPayload.meshInstanceGroups.reserve(sceneAssetPayload.meshInstanceGroups.size() + sceneManifest.instanceGroups.size());

		for (const CookedSceneInstanceRecord& instanceRecord : sceneManifest.instances)
		{
			if (instanceRecord.meshAssetIndex >= sceneManifest.meshAssetReferences.size())
			{
				errorMessage = std::format(
				    "Scene manifest '{}' references mesh index {} but only {} mesh assets were declared",
				    sceneAssetId.value,
				    instanceRecord.meshAssetIndex,
				    sceneManifest.meshAssetReferences.size());
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

			SceneAssetPayload::MeshInstance meshInstance;
			meshInstance.meshAssetIndex = meshAssetBaseIndex + instanceRecord.meshAssetIndex;
			meshInstance.transform = Transform(DirectX::XMLoadFloat4x4(&instanceRecord.worldTransform));
			meshInstance.material = instanceRecord.materialAssetIndex == kInvalidCookedMaterialAssetIndex
			                            ? MaterialHandle::Invalid()
			                            : MaterialHandle(materialBaseIndex + instanceRecord.materialAssetIndex);
			meshInstance.groupIndex = instanceRecord.groupIndex == kInvalidCookedSceneInstanceGroupIndex
			                              ? kInvalidSceneMeshInstanceGroupIndex
			                              : groupBaseIndex + instanceRecord.groupIndex;
			sceneAssetPayload.meshInstances.push_back(std::move(meshInstance));
		}

		for (const CookedSceneInstanceGroupRecord& groupRecord : sceneManifest.instanceGroups)
		{
			if (groupRecord.meshAssetIndex >= sceneManifest.meshAssetReferences.size())
			{
				errorMessage = std::format(
				    "Scene manifest '{}' references mesh group mesh index {} but only {} mesh assets were declared",
				    sceneAssetId.value,
				    groupRecord.meshAssetIndex,
				    sceneManifest.meshAssetReferences.size());
				return false;
			}

			if (groupRecord.materialAssetIndex != kInvalidCookedMaterialAssetIndex &&
			    groupRecord.materialAssetIndex >= sceneManifest.materialAssetReferences.size())
			{
				errorMessage = std::format(
				    "Scene manifest '{}' references mesh group material index {} but only {} material assets were loaded",
				    sceneAssetId.value,
				    groupRecord.materialAssetIndex,
				    sceneManifest.materialAssetReferences.size());
				return false;
			}

			SceneAssetPayload::MeshInstanceGroup meshInstanceGroup;
			meshInstanceGroup.meshAssetIndex = meshAssetBaseIndex + groupRecord.meshAssetIndex;
			meshInstanceGroup.material = groupRecord.materialAssetIndex == kInvalidCookedMaterialAssetIndex
			                                  ? MaterialHandle::Invalid()
			                                  : MaterialHandle(materialBaseIndex + groupRecord.materialAssetIndex);
			meshInstanceGroup.firstInstance = instanceBaseIndex + groupRecord.firstInstance;
			meshInstanceGroup.instanceCount = groupRecord.instanceCount;
			meshInstanceGroup.groupKind = ToSceneMeshInstanceGroupKind(groupRecord.groupKind);
			meshInstanceGroup.flags = groupRecord.flags;
			sceneAssetPayload.meshInstanceGroups.push_back(meshInstanceGroup);
		}

		sceneAssetPayload.cameras.reserve(sceneAssetPayload.cameras.size() + sceneManifest.cameras.size());
		for (std::size_t cameraIndex = 0; cameraIndex < sceneManifest.cameras.size(); ++cameraIndex)
		{
			sceneAssetPayload.cameras.push_back(BuildSceneAssetCamera(sceneManifest.cameras[cameraIndex], cameraIndex));
		}

		sceneAssetPayload.lights.reserve(sceneAssetPayload.lights.size() + sceneManifest.lights.size());
		for (std::size_t lightIndex = 0; lightIndex < sceneManifest.lights.size(); ++lightIndex)
		{
			sceneAssetPayload.lights.push_back(BuildSceneAssetLight(sceneManifest.lights[lightIndex], lightIndex));
		}

		sceneAssetPayload.diagnostics.loadedSceneAssetCount += 1u;
		sceneAssetPayload.diagnostics.meshAssetReferenceCount += sceneManifest.meshAssetReferences.size();
		sceneAssetPayload.diagnostics.meshInstanceCount += sceneManifest.instances.size();
		sceneAssetPayload.diagnostics.meshInstanceGroupCount += sceneManifest.instanceGroups.size();
		sceneAssetPayload.diagnostics.cameraCount += sceneManifest.cameras.size();
		sceneAssetPayload.diagnostics.lightCount += sceneManifest.lights.size();

		SPDLOG_LOGGER_INFO(
		    g_sceneAssetManagerLogger,
		    "SceneAssetManager: Loaded scene asset '{}' - meshAssetRefs={}, meshInstances={}, instanceGroups={}, materials={}, cameras={}, lights={}",
		    sceneAssetId.value,
		    sceneManifest.meshAssetReferences.size(),
		    sceneManifest.instances.size(),
		    sceneManifest.instanceGroups.size(),
		    sceneManifest.materialAssetReferences.size(),
		    sceneManifest.cameras.size(),
		    sceneManifest.lights.size());

		materialBaseIndex += static_cast<std::uint32_t>(sceneManifest.materialAssetReferences.size());
		errorMessage.clear();
		return true;
	}

}
