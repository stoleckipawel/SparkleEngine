#include "PCH.h"

#include "Assets/SceneAssetManager.h"

#include "Assets/SceneAssetRegistry.h"
#include "Assets/SceneAssetPayloadLoader.h"
#include "Core/Public/Paths/DirectoryPaths.h"

#include <format>

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

		const auto manifestRelativePath = m_sceneAssetRegistry.Resolve(sceneAssetId.value);
		if (!manifestRelativePath)
		{
			errorMessage = std::format("Scene asset id '{}' is not registered in the cooked scene asset registry", sceneAssetId.value);
			return false;
		}

		return SceneAssetPayloadLoader::AppendSceneAsset(
		    sceneAssetId,
		    *manifestRelativePath,
		    sceneAssetPayload,
		    materialBaseIndex,
		    errorMessage);
	}

}
