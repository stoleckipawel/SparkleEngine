#include "PCH.h"
#include "Core/Public/FileSystemUtils.h"

#include "Assets/SceneAssetRegistry.h"

#include "Core/Public/Strings/StringUtils.h"

#include <fstream>

class SceneAssetRegistryOperations final
{
  public:
	static constexpr std::string_view kRegistryHeader = "[SceneAssetRegistry]";
	static constexpr std::string_view kEntriesHeader = "[Entries]";

	static bool TryParseRegistryEntry(std::string_view entryValue, std::string& outSceneAssetId, std::filesystem::path& outManifestRelativePath)
	{
		const std::size_t separatorIndex = entryValue.find('|');
		if (separatorIndex == std::string_view::npos)
		{
			return false;
		}

		outSceneAssetId = Strings::TrimCopy(entryValue.substr(0, separatorIndex));
		outManifestRelativePath = Strings::TrimCopy(entryValue.substr(separatorIndex + 1));
		return !outSceneAssetId.empty() && !outManifestRelativePath.empty();
	}
};

namespace Assets
{
	bool SceneAssetRegistry::Load(std::string& outErrorMessage)
	{
		m_entries.clear();

		const std::filesystem::path registryPath = Filesystem::GetSceneAssetRegistryPath();
		std::error_code errorCode;
		if (!std::filesystem::exists(registryPath, errorCode))
		{
			outErrorMessage.clear();
			return true;
		}

		std::ifstream input(registryPath);
		if (!input.is_open())
		{
			outErrorMessage = "Failed to open scene asset registry '" + registryPath.string() + "'";
			return false;
		}

		bool inEntriesSection = false;
		for (std::string line; std::getline(input, line);)
		{
			const std::string trimmedLine = Strings::TrimCopy(line);
			if (trimmedLine.empty() || trimmedLine[0] == '#' || trimmedLine[0] == ';')
			{
				continue;
			}

			if (trimmedLine == SceneAssetRegistryOperations::kRegistryHeader)
			{
				inEntriesSection = false;
				continue;
			}

			if (trimmedLine == SceneAssetRegistryOperations::kEntriesHeader)
			{
				inEntriesSection = true;
				continue;
			}

			std::string_view key;
			std::string_view value;
			if (!Strings::TrySplitKeyValue(trimmedLine, '=', key, value))
			{
				continue;
			}

			if (!inEntriesSection || !Strings::EqualsIgnoreCase(key, "Entry"))
			{
				continue;
			}

			std::string sceneAssetId;
			std::filesystem::path manifestRelativePath;
			if (!SceneAssetRegistryOperations::TryParseRegistryEntry(value, sceneAssetId, manifestRelativePath))
			{
				outErrorMessage = "Failed to parse scene asset registry entry in '" + registryPath.string() + "'";
				m_entries.clear();
				return false;
			}

			m_entries[sceneAssetId] = manifestRelativePath;
		}

		outErrorMessage.clear();
		return true;
	}

	bool SceneAssetRegistry::Save(
	    const std::filesystem::path& outputPath,
	    std::string& outErrorMessage) const
	{
		std::error_code errorCode;
		std::filesystem::create_directories(outputPath.parent_path(), errorCode);
		if (errorCode)
		{
			outErrorMessage =
			    "Failed to create scene asset registry directory '" +
			    outputPath.parent_path().string() + "'";
			return false;
		}

		std::ofstream output(outputPath, std::ios::trunc);
		if (!output.is_open())
		{
			outErrorMessage =
			    "Failed to open scene asset registry for writing '" +
			    outputPath.string() + "'";
			return false;
		}

		output << SceneAssetRegistryOperations::kRegistryHeader << "\n";
		output << "Version = " << kSceneAssetRegistryVersion << "\n\n";
		output << SceneAssetRegistryOperations::kEntriesHeader << "\n";
		for (const auto& [sceneAssetId, manifestRelativePath] : m_entries)
		{
			output << "Entry = " << sceneAssetId << "|" << manifestRelativePath.generic_string() << "\n";
		}

		if (!output.good())
		{
			outErrorMessage =
			    "Failed while writing scene asset registry '" +
			    outputPath.string() + "'";
			return false;
		}

		outErrorMessage.clear();
		return true;
	}

	void SceneAssetRegistry::Upsert(std::string sceneAssetId, std::filesystem::path sceneManifestRelativePath)
	{
		m_entries[std::move(sceneAssetId)] = std::move(sceneManifestRelativePath);
	}

	std::map<std::string, std::filesystem::path, std::less<>> SceneAssetRegistry::ReleaseEntries() noexcept
	{
		return std::move(m_entries);
	}
}
