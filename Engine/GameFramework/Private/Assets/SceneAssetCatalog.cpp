#include "PCH.h"

#include "Assets/SceneAssetCatalog.h"

#include "Assets/SceneAssetRegistry.h"
namespace Assets
{
	std::optional<std::filesystem::path> SceneAssetCatalog::Resolve(std::string_view sceneAssetId) const
	{
		const auto entry = m_entries.find(sceneAssetId);
		return entry == m_entries.end() ? std::nullopt : std::optional<std::filesystem::path>(entry->second);
	}

	std::shared_ptr<const SceneAssetCatalog> LoadSceneAssetCatalog(std::uint64_t generation)
	{
		SceneAssetRegistry registry;
		registry.Load();
		return std::make_shared<const SceneAssetCatalog>(generation, registry.ReleaseEntries());
	}
}
