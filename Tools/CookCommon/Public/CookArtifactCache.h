#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace Cook
{
	struct CookArtifactKey final
	{
		std::string assetType;
		std::string assetId;
		std::string cookerName;
		std::filesystem::path outputPath;
		std::uint32_t cookedFormatVersion = 0;
		std::uint32_t cookerVersion = 0;
		std::uint64_t sourceHash = 0;
		std::uint64_t dependencyHash = 0;
		std::uint64_t settingsHash = 0;

		bool IsValid() const noexcept;
		std::string BuildCanonicalString() const;
		std::uint64_t ComputeIdentityHash() const;
	};

	class CookArtifactCache final
	{
	  public:
		CookArtifactCache() = delete;

		static std::uint64_t ComputeSettingsHash(std::string_view canonicalSettings) noexcept;

		static std::filesystem::path MetadataPathForOutput(const std::filesystem::path& outputPath);
		static bool IsCurrent(const CookArtifactKey& key, std::string& outErrorMessage);
		static bool Publish(const CookArtifactKey& key, std::string& outErrorMessage);
	};
}  // namespace Cook
