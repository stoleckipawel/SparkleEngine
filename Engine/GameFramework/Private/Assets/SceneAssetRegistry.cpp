#include "PCH.h"
#include "Core/Public/FileSystemUtils.h"

#include "Assets/SceneAssetRegistry.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Strings/StringUtils.h"

#include <fstream>
#include <format>
#include <utility>

namespace Assets
{
	constexpr std::string_view kSceneAssetRegistryHeader = "[SceneAssetRegistry]";
	constexpr std::string_view kSceneAssetEntriesHeader = "[Entries]";

	using SceneAssetRegistryEntries = std::map<std::string, std::filesystem::path, std::less<>>;

	class SceneAssetRegistryReader final
	{
	  public:
		explicit SceneAssetRegistryReader(std::filesystem::path registryPath);

		SceneAssetRegistryEntries Read() const;

	  private:
		SceneAssetRegistryEntries ParseEntries(std::istream& input) const;
		std::pair<std::string, std::filesystem::path> ParseEntry(std::string_view value, std::size_t lineNumber) const;

		std::filesystem::path m_registryPath;
	};

	SceneAssetRegistryReader::SceneAssetRegistryReader(std::filesystem::path registryPath) :
	    m_registryPath(std::move(registryPath))
	{
	}

	SceneAssetRegistryEntries SceneAssetRegistryReader::Read() const
	{
		std::error_code errorCode;
		const bool exists = std::filesystem::exists(m_registryPath, errorCode);
		if (errorCode)
		{
			throw Diagnostics::Error(
			    std::format("Could not inspect scene asset registry '{}': {}.", m_registryPath.string(), errorCode.message()));
		}
		if (!exists)
		{
			return {};
		}

		std::ifstream input(m_registryPath);
		if (!input.is_open())
		{
			throw Diagnostics::Error("Could not open scene asset registry '" + m_registryPath.string() + "'.");
		}

		return ParseEntries(input);
	}

	SceneAssetRegistryEntries SceneAssetRegistryReader::ParseEntries(std::istream& input) const
	{
		SceneAssetRegistryEntries entries;
		bool foundRegistryHeader = false;
		bool foundEntriesHeader = false;
		bool inEntriesSection = false;
		std::size_t lineNumber = 0;
		for (std::string line; std::getline(input, line);)
		{
			++lineNumber;
			const std::string trimmedLine = Strings::TrimCopy(line);
			if (trimmedLine.empty() || trimmedLine[0] == '#' || trimmedLine[0] == ';')
			{
				continue;
			}

			if (trimmedLine == kSceneAssetRegistryHeader)
			{
				if (foundRegistryHeader)
				{
					throw Diagnostics::Error(
					    std::format("Scene asset registry '{}' repeats its header at line {}.", m_registryPath.string(), lineNumber));
				}
				foundRegistryHeader = true;
				inEntriesSection = false;
				continue;
			}

			if (trimmedLine == kSceneAssetEntriesHeader)
			{
				if (!foundRegistryHeader || foundEntriesHeader)
				{
					throw Diagnostics::Error(
					    std::format("Scene asset registry '{}' has an invalid entries section at line {}.", m_registryPath.string(), lineNumber));
				}
				foundEntriesHeader = true;
				inEntriesSection = true;
				continue;
			}

			std::string_view key;
			std::string_view value;
			if (!inEntriesSection || !Strings::TrySplitKeyValue(trimmedLine, '=', key, value) ||
			    !Strings::EqualsIgnoreCase(key, "Entry"))
			{
				throw Diagnostics::Error(
				    std::format("Scene asset registry '{}' has an invalid field at line {}.", m_registryPath.string(), lineNumber));
			}

			auto [sceneAssetId, manifestRelativePath] = ParseEntry(value, lineNumber);
			if (!entries.emplace(std::move(sceneAssetId), std::move(manifestRelativePath)).second)
			{
				throw Diagnostics::Error(
				    std::format("Scene asset registry '{}' repeats an asset identity at line {}.", m_registryPath.string(), lineNumber));
			}
		}

		if (!foundRegistryHeader || !foundEntriesHeader)
		{
			throw Diagnostics::Error("Scene asset registry '" + m_registryPath.string() + "' has an incomplete structure.");
		}
		return entries;
	}

	std::pair<std::string, std::filesystem::path> SceneAssetRegistryReader::ParseEntry(
	    std::string_view entryValue,
	    std::size_t lineNumber) const
	{
		const std::size_t separatorIndex = entryValue.find('|');
		if (separatorIndex == std::string_view::npos || entryValue.find('|', separatorIndex + 1) != std::string_view::npos)
		{
			throw Diagnostics::Error(
			    std::format("Scene asset registry '{}' has an invalid entry at line {}.", m_registryPath.string(), lineNumber));
		}

		std::string sceneAssetId = Strings::TrimCopy(entryValue.substr(0, separatorIndex));
		std::filesystem::path manifestRelativePath = Strings::TrimCopy(entryValue.substr(separatorIndex + 1));
		if (sceneAssetId.empty() || manifestRelativePath.empty() || manifestRelativePath.is_absolute())
		{
			throw Diagnostics::Error(
			    std::format("Scene asset registry '{}' has an invalid entry at line {}.", m_registryPath.string(), lineNumber));
		}
		for (const std::filesystem::path& component : manifestRelativePath)
		{
			if (component == "..")
			{
				throw Diagnostics::Error(
				    std::format("Scene asset registry '{}' escapes its asset root at line {}.", m_registryPath.string(), lineNumber));
			}
		}
		return {std::move(sceneAssetId), std::move(manifestRelativePath)};
	}

	void SceneAssetRegistry::Load()
	{
		m_entries = SceneAssetRegistryReader(Filesystem::GetSceneAssetRegistryPath()).Read();
	}

	void SceneAssetRegistry::Save(const std::filesystem::path& outputPath) const
	{
		if (outputPath.empty())
		{
			throw Diagnostics::Error("Scene asset registry output path is empty.");
		}

		std::error_code errorCode;
		if (!outputPath.parent_path().empty())
		{
			std::filesystem::create_directories(outputPath.parent_path(), errorCode);
		}
		if (errorCode)
		{
			throw Diagnostics::Error(
			    std::format("Could not create scene asset registry directory '{}': {}.", outputPath.parent_path().string(), errorCode.message()));
		}

		std::ofstream output(outputPath, std::ios::trunc);
		if (!output.is_open())
		{
			throw Diagnostics::Error("Could not open scene asset registry for writing '" + outputPath.string() + "'.");
		}

		output << kSceneAssetRegistryHeader << "\n\n";
		output << kSceneAssetEntriesHeader << "\n";
		for (const auto& [sceneAssetId, manifestRelativePath] : m_entries)
		{
			output << "Entry = " << sceneAssetId << "|" << manifestRelativePath.generic_string() << "\n";
		}

		if (!output.good())
		{
			throw Diagnostics::Error("Could not write scene asset registry '" + outputPath.string() + "'.");
		}
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
