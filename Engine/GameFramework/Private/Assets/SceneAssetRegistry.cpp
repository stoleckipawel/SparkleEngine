#include "PCH.h"

#include "Assets/SceneAssetRegistry.h"

#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Strings/StringUtils.h"

#include <fstream>

namespace
{
	constexpr std::string_view kRegistryHeader = "[SceneAssetRegistry]";
	constexpr std::string_view kEntriesHeader = "[Entries]";

	bool TryParseRegistryEntry(std::string_view entryValue, std::string& outSceneAssetId, std::filesystem::path& outManifestRelativePath)
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
}

namespace Assets
{
	bool SceneAssetRegistry::Load(std::string& outErrorMessage)
	{
		m_entries.clear();

		const std::filesystem::path registryPath = Paths::SceneAssetRegistry();
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

			if (trimmedLine == kRegistryHeader)
			{
				inEntriesSection = false;
				continue;
			}

			if (trimmedLine == kEntriesHeader)
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
			if (!TryParseRegistryEntry(value, sceneAssetId, manifestRelativePath))
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

	bool SceneAssetRegistry::Save(std::string& outErrorMessage) const
	{
		const std::filesystem::path registryPath = Paths::SceneAssetRegistry();
		std::error_code errorCode;
		std::filesystem::create_directories(registryPath.parent_path(), errorCode);
		if (errorCode)
		{
			outErrorMessage = "Failed to create scene asset registry directory '" + registryPath.parent_path().string() + "'";
			return false;
		}

		std::ofstream output(registryPath, std::ios::trunc);
		if (!output.is_open())
		{
			outErrorMessage = "Failed to open scene asset registry for writing '" + registryPath.string() + "'";
			return false;
		}

		output << kRegistryHeader << "\n";
		output << "Version = " << kSceneAssetRegistryVersion << "\n\n";
		output << kEntriesHeader << "\n";
		for (const auto& [sceneAssetId, manifestRelativePath] : m_entries)
		{
			output << "Entry = " << sceneAssetId << "|" << manifestRelativePath.generic_string() << "\n";
		}

		if (!output.good())
		{
			outErrorMessage = "Failed while writing scene asset registry '" + registryPath.string() + "'";
			return false;
		}

		outErrorMessage.clear();
		return true;
	}

	void SceneAssetRegistry::Clear() noexcept
	{
		m_entries.clear();
	}

	void SceneAssetRegistry::Upsert(std::string sceneAssetId, std::filesystem::path sceneManifestRelativePath)
	{
		m_entries[std::move(sceneAssetId)] = std::move(sceneManifestRelativePath);
	}

	std::optional<std::filesystem::path> SceneAssetRegistry::Resolve(std::string_view sceneAssetId) const
	{
		if (const auto it = m_entries.find(std::string(sceneAssetId)); it != m_entries.end())
		{
			return it->second;
		}

		return std::nullopt;
	}
}