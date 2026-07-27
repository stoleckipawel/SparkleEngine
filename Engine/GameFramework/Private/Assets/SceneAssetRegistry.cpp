#include "PCH.h"
#include "Core/Public/FileSystemUtils.h"

#include "Assets/SceneAssetRegistry.h"

#include "Core/Public/Strings/StringUtils.h"

#include <fstream>
#include <utility>

namespace Assets
{
	constexpr std::string_view kSceneAssetRegistryHeader = "[SceneAssetRegistry]";
	constexpr std::string_view kSceneAssetEntriesHeader = "[Entries]";

	class SceneAssetRegistryReader final
	{
	  public:
		explicit SceneAssetRegistryReader(std::filesystem::path registryPath);

		bool Read(
		    std::map<std::string, std::filesystem::path, std::less<>>& outEntries,
		    std::string& outErrorMessage) const;

	  private:
		bool ParseEntries(
		    std::istream& input,
		    std::map<std::string, std::filesystem::path, std::less<>>& outEntries,
		    std::string& outErrorMessage) const;
		bool ParseEntry(
		    std::string_view value,
		    std::map<std::string, std::filesystem::path, std::less<>>& outEntries,
		    std::string& outErrorMessage) const;
		static bool TryParseEntry(
		    std::string_view entryValue,
		    std::string& outSceneAssetId,
		    std::filesystem::path& outManifestRelativePath);

		std::filesystem::path m_registryPath;
	};

	SceneAssetRegistryReader::SceneAssetRegistryReader(std::filesystem::path registryPath) :
	    m_registryPath(std::move(registryPath))
	{
	}

	bool SceneAssetRegistryReader::Read(
	    std::map<std::string, std::filesystem::path, std::less<>>& outEntries,
	    std::string& outErrorMessage) const
	{
		std::error_code errorCode;
		if (!std::filesystem::exists(m_registryPath, errorCode))
		{
			outErrorMessage.clear();
			return true;
		}

		std::ifstream input(m_registryPath);
		if (!input.is_open())
		{
			outErrorMessage = "Failed to open scene asset registry '" + m_registryPath.string() + "'";
			return false;
		}

		return ParseEntries(input, outEntries, outErrorMessage);
	}

	bool SceneAssetRegistryReader::ParseEntries(
	    std::istream& input,
	    std::map<std::string, std::filesystem::path, std::less<>>& outEntries,
	    std::string& outErrorMessage) const
	{
		bool inEntriesSection = false;
		for (std::string line; std::getline(input, line);)
		{
			const std::string trimmedLine = Strings::TrimCopy(line);
			if (trimmedLine.empty() || trimmedLine[0] == '#' || trimmedLine[0] == ';')
			{
				continue;
			}

			if (trimmedLine == kSceneAssetRegistryHeader)
			{
				inEntriesSection = false;
				continue;
			}

			if (trimmedLine == kSceneAssetEntriesHeader)
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

			if (!ParseEntry(value, outEntries, outErrorMessage))
			{
				return false;
			}
		}

		outErrorMessage.clear();
		return true;
	}

	bool SceneAssetRegistryReader::ParseEntry(
	    std::string_view value,
	    std::map<std::string, std::filesystem::path, std::less<>>& outEntries,
	    std::string& outErrorMessage) const
	{
		std::string sceneAssetId;
		std::filesystem::path manifestRelativePath;
		if (!TryParseEntry(value, sceneAssetId, manifestRelativePath))
		{
			outErrorMessage = "Failed to parse scene asset registry entry in '" + m_registryPath.string() + "'";
			return false;
		}

		outEntries[std::move(sceneAssetId)] = std::move(manifestRelativePath);
		return true;
	}

	bool SceneAssetRegistryReader::TryParseEntry(
	    std::string_view entryValue,
	    std::string& outSceneAssetId,
	    std::filesystem::path& outManifestRelativePath)
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

	bool SceneAssetRegistry::Load(std::string& outErrorMessage)
	{
		std::map<std::string, std::filesystem::path, std::less<>> entries;
		if (!SceneAssetRegistryReader(Filesystem::GetSceneAssetRegistryPath()).Read(entries, outErrorMessage))
		{
			return false;
		}

		m_entries = std::move(entries);
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

		output << kSceneAssetRegistryHeader << "\n\n";
		output << kSceneAssetEntriesHeader << "\n";
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
