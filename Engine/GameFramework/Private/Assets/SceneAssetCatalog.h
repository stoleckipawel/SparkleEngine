#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace Assets
{
	class SceneAssetCatalog final
	{
	  public:
		SceneAssetCatalog(std::uint64_t generation, std::map<std::string, std::filesystem::path, std::less<>> entries) :
		    m_generation(generation), m_entries(std::move(entries))
		{
		}

		std::uint64_t GetGeneration() const noexcept { return m_generation; }
		std::optional<std::filesystem::path> Resolve(std::string_view sceneAssetId) const;

	  private:
		std::uint64_t m_generation = 0;
		std::map<std::string, std::filesystem::path, std::less<>> m_entries;
	};

	std::shared_ptr<const SceneAssetCatalog> LoadSceneAssetCatalog(std::uint64_t generation);
}
