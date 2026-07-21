#include "PCH.h"
#include "Core/Public/FileSystemUtils.h"

#include "TextureDiagnosticMetadataCatalog.h"

#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Json/JsonReader.h"
#include "Core/Public/Strings/StringUtils.h"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace
{
struct TextureDiagnosticMetadataCache final
{
	std::unordered_map<std::uint64_t, TextureDiagnosticMetadata> ByAssetId;
	std::unordered_map<std::string, TextureDiagnosticMetadata> ByCookedPath;
};

bool EndsWithIgnoreCase(std::string_view value, std::string_view suffix) noexcept
{
	if (suffix.size() > value.size())
	{
		return false;
	}
	return Strings::EqualsIgnoreCase(value.substr(value.size() - suffix.size()), suffix);
}

std::optional<std::filesystem::path> FindLatestTextureCookSummary()
{
	const std::filesystem::path summaryRoot = Filesystem::GetWorkspaceRootPath() / "artifacts" / "diagnostics" / "cook" / "Summaries";
	std::error_code errorCode;
	if (!std::filesystem::exists(summaryRoot, errorCode) || errorCode)
	{
		return std::nullopt;
	}

	std::optional<std::filesystem::path> latestPath;
	std::filesystem::file_time_type latestWriteTime{};
	for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(summaryRoot, errorCode))
	{
		if (errorCode)
		{
			break;
		}
		if (!entry.is_regular_file(errorCode) || errorCode)
		{
			errorCode.clear();
			continue;
		}

		const std::filesystem::path& path = entry.path();
		if (!EndsWithIgnoreCase(path.filename().generic_string(), "-texturecook-summary.json"))
		{
			continue;
		}

		const std::filesystem::file_time_type writeTime = entry.last_write_time(errorCode);
		if (errorCode)
		{
			errorCode.clear();
			continue;
		}

		if (!latestPath || writeTime > latestWriteTime)
		{
			latestPath = path;
			latestWriteTime = writeTime;
		}
	}

	return latestPath;
}

std::string_view FindJsonArray(std::string_view document, std::string_view key) noexcept
{
	std::size_t cursor = Json::FindPropertyValue(document, key);
	if (cursor == std::string_view::npos || cursor >= document.size() || document[cursor] != '[')
	{
		return {};
	}

	const std::size_t arrayStart = cursor;
	int depth = 0;
	bool inString = false;
	bool escaping = false;
	for (; cursor < document.size(); ++cursor)
	{
		const char character = document[cursor];
		if (inString)
		{
			if (escaping)
			{
				escaping = false;
				continue;
			}
			if (character == '\\')
			{
				escaping = true;
				continue;
			}
			if (character == '"')
			{
				inString = false;
			}
			continue;
		}

		if (character == '"')
		{
			inString = true;
			continue;
		}
		if (character == '[')
		{
			++depth;
			continue;
		}
		if (character == ']')
		{
			--depth;
			if (depth == 0)
			{
				return document.substr(arrayStart, cursor - arrayStart + 1u);
			}
		}
	}

	return {};
}

void ParseTextureCookSummaryArray(
    std::string_view arrayText,
    TextureDiagnosticMetadataCache& outCache)
{
	std::size_t cursor = 0;
	while (cursor < arrayText.size())
	{
		const std::size_t objectStart = arrayText.find('{', cursor);
		if (objectStart == std::string_view::npos)
		{
			return;
		}
		const std::size_t objectEnd = arrayText.find('}', objectStart + 1u);
		if (objectEnd == std::string_view::npos)
		{
			return;
		}

		const std::string_view objectText = arrayText.substr(objectStart, objectEnd - objectStart + 1u);
		std::string assetIdText;
		std::string displayName;
		std::string sourcePath;
		std::string outputPath;
		std::uint64_t assetId = 0;
		if (Json::TryReadStringProperty(objectText, "assetId", assetIdText) && Formatting::TryParseHexUInt64(assetIdText, assetId))
		{
			Json::TryReadStringProperty(objectText, "name", displayName);
			Json::TryReadStringProperty(objectText, "source", sourcePath);
			Json::TryReadStringProperty(objectText, "output", outputPath);
			TextureDiagnosticMetadata metadata{std::move(displayName), std::move(sourcePath)};
			outCache.ByAssetId[assetId] = metadata;
			if (!outputPath.empty())
			{
				const std::filesystem::path cookedPath{outputPath};
				outCache.ByCookedPath[cookedPath.generic_string()] = metadata;
				outCache.ByCookedPath[cookedPath.filename().generic_string()] = std::move(metadata);
			}
		}

		cursor = objectEnd + 1u;
	}
}

TextureDiagnosticMetadataCache LoadTextureDiagnosticMetadata()
{
	TextureDiagnosticMetadataCache cache;
	const std::optional<std::filesystem::path> summaryPath = FindLatestTextureCookSummary();
	if (!summaryPath)
	{
		return cache;
	}

	std::string summaryText;
	std::string readError;
	if (!Files::TryReadAllText(*summaryPath, summaryText, readError))
	{
		return cache;
	}

	std::string schema;
	if (!Json::TryReadStringProperty(summaryText, "schema", schema) || schema != "texture-cooker-summary-v1")
	{
		return cache;
	}

	std::string_view requests = FindJsonArray(summaryText, "allRequests");
	if (requests.empty())
	{
		requests = FindJsonArray(summaryText, "topRequests");
	}
	ParseTextureCookSummaryArray(requests, cache);
	return cache;
}

std::optional<std::uint64_t> TryParseCookedTextureAssetId(std::string_view key) noexcept
{
	const std::filesystem::path path{std::string(key)};
	if (!Strings::EqualsIgnoreCase(path.extension().string(), ".stex"))
	{
		return std::nullopt;
	}

	std::uint64_t assetId = 0;
	const std::string stem = path.stem().string();
	if (Formatting::TryParseHexUInt64(stem, assetId))
	{
		return assetId;
	}

	const std::size_t suffixSeparator = stem.find_last_of('_');
	if (suffixSeparator == std::string::npos || suffixSeparator + 1u >= stem.size())
	{
		return std::nullopt;
	}
	if (!Formatting::TryParseHexUInt64(std::string_view(stem).substr(suffixSeparator + 1u), assetId))
	{
		return std::nullopt;
	}
	return assetId;
}

}

std::optional<TextureDiagnosticMetadata> FindTextureDiagnosticMetadata(const TextureDiagnosticsRow& row)
{
	static const TextureDiagnosticMetadataCache metadata = LoadTextureDiagnosticMetadata();
	const std::filesystem::path rowPath{row.Key};
	if (const auto metadataIt = metadata.ByCookedPath.find(rowPath.generic_string()); metadataIt != metadata.ByCookedPath.end())
	{
		return metadataIt->second;
	}
	if (const auto metadataIt = metadata.ByCookedPath.find(rowPath.filename().generic_string()); metadataIt != metadata.ByCookedPath.end())
	{
		return metadataIt->second;
	}

	const std::optional<std::uint64_t> assetId = TryParseCookedTextureAssetId(row.Key);
	if (!assetId)
	{
		return std::nullopt;
	}

	const auto metadataIt = metadata.ByAssetId.find(*assetId);
	return metadataIt != metadata.ByAssetId.end() ? std::optional<TextureDiagnosticMetadata>(metadataIt->second) : std::nullopt;
}
