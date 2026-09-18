#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <stop_token>
#include <string>
#include <vector>

struct ArtifactBundleFile final
{
	std::filesystem::path RelativePath;
	std::vector<std::byte> Bytes;
};

struct ArtifactBundlePublicationRequest final
{
	std::filesystem::path AllowedRoot;
	std::filesystem::path PublicationDirectory;
	std::filesystem::path CompletionFile;
	std::uint64_t MaximumOutputBytes = 0u;
	std::vector<ArtifactBundleFile> Files;
};

struct ArtifactBundlePublishedFile final
{
	std::filesystem::path RelativePath;
	std::uint64_t Size = 0u;
	std::string Sha256;
};

struct ArtifactBundlePublicationResult final
{
	bool Succeeded = false;
	std::filesystem::path PublicationDirectory;
	std::vector<ArtifactBundlePublishedFile> Files;
	std::string ErrorMessage;
};

class ArtifactBundlePublication final
{
public:
	static ArtifactBundlePublicationResult Publish(ArtifactBundlePublicationRequest request, std::stop_token cancellationToken) noexcept;

private:
	static std::uintmax_t RequiredFreeSpace(std::uintmax_t publicationBytes) noexcept;
	static bool IsRelativeFilePath(const std::filesystem::path& path) noexcept;
	static bool WriteFile(
	    const std::filesystem::path& stagingDirectory,
	    const ArtifactBundleFile& file,
	    ArtifactBundlePublishedFile& published,
	    std::string& errorMessage);
};
