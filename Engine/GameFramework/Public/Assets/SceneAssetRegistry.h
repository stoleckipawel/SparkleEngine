#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace Assets
{
	inline constexpr std::uint32_t kSceneAssetRegistryVersion = 1;

	class SPARKLE_ENGINE_API SceneAssetRegistry final
	{
	  public:
		bool Load(std::string& outErrorMessage);
		bool Save(std::string& outErrorMessage) const;
		void Clear() noexcept;
		void Upsert(std::string sceneAssetId, std::filesystem::path sceneManifestRelativePath);
		std::optional<std::filesystem::path> Resolve(std::string_view sceneAssetId) const;
	  private:
		std::map<std::string, std::filesystem::path, std::less<>> m_entries;
	};
}