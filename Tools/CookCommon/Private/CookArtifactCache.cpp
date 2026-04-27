#include "CookArtifactCache.h"

#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Hash/HashUtils.h"

#include <charconv>
#include <format>
#include <fstream>
#include <sstream>
#include <string_view>
#include <system_error>

namespace Cook
{
	static constexpr std::string_view kCookArtifactMetadataHeader = "CookArtifact|1";

	static std::string FormatHex(std::uint64_t value)
	{
		return std::format("{:016X}", value);
	}

	static bool ParseHex(std::string_view value, std::uint64_t& outValue) noexcept
	{
		unsigned long long parsedValue = 0;
		const char* begin = value.data();
		const char* end = value.data() + value.size();
		const auto parseResult = std::from_chars(begin, end, parsedValue, 16);
		if (parseResult.ec != std::errc() || parseResult.ptr != end)
		{
			return false;
		}

		outValue = static_cast<std::uint64_t>(parsedValue);
		return true;
	}

	static bool ReadIdentityHash(
	    const std::filesystem::path& metadataPath,
	    std::uint64_t& outIdentityHash,
	    std::string& outErrorMessage)
	{
		std::ifstream input(metadataPath);
		if (!input.is_open())
		{
			outIdentityHash = 0;
			outErrorMessage.clear();
			return true;
		}

		std::string line;
		if (!std::getline(input, line) || line != kCookArtifactMetadataHeader)
		{
			outErrorMessage = "Cook artifact metadata has an invalid header: '" + metadataPath.string() + "'";
			return false;
		}

		while (std::getline(input, line))
		{
			constexpr std::string_view kIdentityHashPrefix = "IdentityHash=";
			if (line.starts_with(kIdentityHashPrefix))
			{
				const std::string_view value(line.data() + kIdentityHashPrefix.size(), line.size() - kIdentityHashPrefix.size());
				if (!ParseHex(value, outIdentityHash))
				{
					outErrorMessage = "Cook artifact metadata has an invalid identity hash: '" + metadataPath.string() + "'";
					return false;
				}

				outErrorMessage.clear();
				return true;
			}
		}

		outErrorMessage = "Cook artifact metadata is missing IdentityHash: '" + metadataPath.string() + "'";
		return false;
	}

	bool CookArtifactKey::IsValid() const noexcept
	{
		return !assetType.empty() && !assetId.empty() && !cookerName.empty() && !outputPath.empty() && cookedFormatVersion != 0 &&
		       cookerVersion != 0 && sourceHash != 0;
	}

	std::string CookArtifactKey::BuildCanonicalString() const
	{
		std::string canonical;
		canonical.reserve(256);
		canonical += "CookArtifact|1|";
		canonical += assetType;
		canonical += '|';
		canonical += assetId;
		canonical += '|';
		canonical += cookerName;
		canonical += '|';
		canonical += std::to_string(cookedFormatVersion);
		canonical += '|';
		canonical += std::to_string(cookerVersion);
		canonical += '|';
		canonical += FormatHex(sourceHash);
		canonical += '|';
		canonical += FormatHex(dependencyHash);
		canonical += '|';
		canonical += FormatHex(settingsHash);
		return canonical;
	}

	std::uint64_t CookArtifactKey::ComputeIdentityHash() const
	{
		const std::uint64_t hash = Hash::Fnv1a64(BuildCanonicalString());
		return hash != 0 ? hash : Hash::kFnv64OffsetBasis;
	}

	std::uint64_t CookArtifactCache::ComputeSettingsHash(std::string_view canonicalSettings) noexcept
	{
		return Hash::FinalizeFnv1a64(Hash::Fnv1a64(canonicalSettings));
	}

	std::filesystem::path CookArtifactCache::MetadataPathForOutput(const std::filesystem::path& outputPath)
	{
		std::filesystem::path metadataPath = outputPath;
		metadataPath += ".cookmeta";
		return metadataPath;
	}

	bool CookArtifactCache::IsCurrent(const CookArtifactKey& key, std::string& outErrorMessage)
	{
		if (!key.IsValid())
		{
			outErrorMessage = "Cook artifact key is invalid.";
			return false;
		}

		std::error_code errorCode;
		if (!std::filesystem::exists(key.outputPath, errorCode) || errorCode)
		{
			outErrorMessage.clear();
			return false;
		}

		const std::filesystem::path metadataPath = MetadataPathForOutput(key.outputPath);
		if (!std::filesystem::exists(metadataPath, errorCode) || errorCode)
		{
			outErrorMessage.clear();
			return false;
		}

		std::uint64_t storedIdentityHash = 0;
		if (!ReadIdentityHash(metadataPath, storedIdentityHash, outErrorMessage))
		{
			return false;
		}

		outErrorMessage.clear();
		return storedIdentityHash == key.ComputeIdentityHash();
	}

	bool CookArtifactCache::Publish(const CookArtifactKey& key, std::string& outErrorMessage)
	{
		if (!key.IsValid())
		{
			outErrorMessage = "Cook artifact key is invalid.";
			return false;
		}

		std::error_code errorCode;
		if (!std::filesystem::exists(key.outputPath, errorCode) || errorCode)
		{
			outErrorMessage = "Cannot publish cook artifact metadata because output is missing: '" + key.outputPath.string() + "'";
			return false;
		}

		std::ostringstream metadata;
		metadata << kCookArtifactMetadataHeader << '\n';
		metadata << "IdentityHash=" << FormatHex(key.ComputeIdentityHash()) << '\n';
		metadata << "AssetType=" << key.assetType << '\n';
		metadata << "AssetId=" << key.assetId << '\n';
		metadata << "CookerName=" << key.cookerName << '\n';
		metadata << "CookedFormatVersion=" << key.cookedFormatVersion << '\n';
		metadata << "CookerVersion=" << key.cookerVersion << '\n';
		metadata << "SourceHash=" << FormatHex(key.sourceHash) << '\n';
		metadata << "DependencyHash=" << FormatHex(key.dependencyHash) << '\n';
		metadata << "SettingsHash=" << FormatHex(key.settingsHash) << '\n';
		metadata << "OutputPath=" << key.outputPath.generic_string() << '\n';

		const std::filesystem::path metadataPath = MetadataPathForOutput(key.outputPath);
		if (!Files::TryWriteAllTextAtomic(metadataPath, metadata.str(), outErrorMessage))
		{
			return false;
		}

		outErrorMessage.clear();
		return true;
	}
}  // namespace Cook
