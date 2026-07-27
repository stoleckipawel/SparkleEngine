#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include <filesystem>
#include <functional>
#include <map>
#include <string>

namespace Assets
{
	class SPARKLE_ENGINE_API SceneAssetRegistry final
	{
	  public:
		bool Load(std::string& outErrorMessage);
		bool Save(const std::filesystem::path& outputPath, std::string& outErrorMessage) const;
		void Upsert(std::string sceneAssetId, std::filesystem::path sceneManifestRelativePath);
		std::map<std::string, std::filesystem::path, std::less<>> ReleaseEntries() noexcept;

	  private:
		std::map<std::string, std::filesystem::path, std::less<>> m_entries;
	};
}
