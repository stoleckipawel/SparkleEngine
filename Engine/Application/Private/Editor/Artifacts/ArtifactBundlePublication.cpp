#include "PCH.h"

#include "Editor/Artifacts/ArtifactBundlePublication.h"

#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/PathUtils.h"

#include <exception>
#include <limits>
#include <unordered_set>
#include <utility>

std::uintmax_t ArtifactBundlePublication::RequiredFreeSpace(std::uintmax_t publicationBytes) noexcept
{
	return publicationBytes * 2u + (publicationBytes + 9u) / 10u;
}

bool ArtifactBundlePublication::IsRelativeFilePath(const std::filesystem::path& path) noexcept
{
	if (path.empty() || path.is_absolute())
	{
		return false;
	}
	for (const std::filesystem::path& component : path)
	{
		if (component == "..")
		{
			return false;
		}
	}
	return path.filename() != ".";
}

bool ArtifactBundlePublication::WriteFile(
    const std::filesystem::path& stagingDirectory,
    const ArtifactBundleFile& file,
    ArtifactBundlePublishedFile& published,
    std::string& errorMessage)
{
	const std::filesystem::path stagedPath = stagingDirectory / file.RelativePath;
	Hash::Sha256Digest sourceHash{};
	Hash::Sha256Digest storedHash{};
	if (!Hash::TrySha256(file.Bytes.data(), file.Bytes.size(), sourceHash, errorMessage)
	    || !Files::TryWriteAllBytes(stagedPath, file.Bytes, errorMessage) || !Hash::TrySha256File(stagedPath, storedHash, errorMessage)
	    || sourceHash != storedHash)
	{
		if (errorMessage.empty())
		{
			errorMessage = "Artifact verification failed after writing.";
		}
		return false;
	}
	published = ArtifactBundlePublishedFile{
	    .RelativePath = file.RelativePath,
	    .Size = static_cast<std::uint64_t>(file.Bytes.size()),
	    .Sha256 = Hash::Sha256ToHex(sourceHash)};
	return true;
}

ArtifactBundlePublicationResult ArtifactBundlePublication::Publish(
    ArtifactBundlePublicationRequest request,
    std::stop_token cancellationToken) noexcept
{
	ArtifactBundlePublicationResult result{.PublicationDirectory = request.PublicationDirectory};
	std::filesystem::path stagingDirectory;
	bool ownsStagingDirectory = false;
	auto fail = [&](std::string message)
	{
		result.ErrorMessage = std::move(message);
		if (ownsStagingDirectory)
		{
			std::error_code cleanupError;
			std::filesystem::remove_all(stagingDirectory, cleanupError);
		}
		return result;
	};

	try
	{
		if (cancellationToken.stop_requested())
		{
			return fail("Artifact publication was cancelled.");
		}
		std::unordered_set<std::string> relativePaths;
		std::uintmax_t publicationBytes = 0u;
		const ArtifactBundleFile* completionFile = nullptr;
		for (const ArtifactBundleFile& file : request.Files)
		{
			const std::string normalizedPath = file.RelativePath.lexically_normal().generic_string();
			if (!IsRelativeFilePath(file.RelativePath) || !relativePaths.insert(normalizedPath).second)
			{
				return fail("Artifact bundle contains an invalid or duplicate file path.");
			}
			if (file.Bytes.size() > (std::numeric_limits<std::uintmax_t>::max)() - publicationBytes)
			{
				return fail("Artifact bundle size overflowed.");
			}
			publicationBytes += file.Bytes.size();
			if (file.RelativePath.lexically_normal() == request.CompletionFile.lexically_normal())
			{
				completionFile = &file;
			}
		}
		if (request.Files.empty() || completionFile == nullptr || publicationBytes > request.MaximumOutputBytes)
		{
			return fail("Artifact bundle is incomplete or exceeds its output budget.");
		}

		std::error_code fileError;
		std::filesystem::create_directories(request.AllowedRoot, fileError);
		if (fileError)
		{
			return fail("Artifact output root is not writable.");
		}
		const std::filesystem::path allowedRoot = std::filesystem::weakly_canonical(request.AllowedRoot, fileError);
		if (fileError)
		{
			return fail("Artifact output root could not be canonicalized.");
		}
		fileError.clear();
		const std::filesystem::path publicationDirectory = std::filesystem::weakly_canonical(request.PublicationDirectory, fileError);
		if (fileError || publicationDirectory == allowedRoot || !Paths::IsUnderRoot(publicationDirectory, allowedRoot)
		    || std::filesystem::exists(publicationDirectory, fileError) || fileError)
		{
			return fail("Artifact publication path is outside its allowed root or already exists.");
		}
		const std::filesystem::space_info space = std::filesystem::space(allowedRoot, fileError);
		if (fileError || space.available < RequiredFreeSpace(publicationBytes))
		{
			return fail("Artifact publication does not have enough free space.");
		}

		stagingDirectory = publicationDirectory;
		stagingDirectory += ".staging";
		if (std::filesystem::exists(stagingDirectory, fileError) || fileError
		    || !std::filesystem::create_directories(stagingDirectory, fileError) || fileError)
		{
			return fail("Artifact staging directory could not be created.");
		}
		ownsStagingDirectory = true;
		result.Files.reserve(request.Files.size());
		for (const ArtifactBundleFile& file : request.Files)
		{
			if (&file == completionFile)
			{
				continue;
			}
			if (cancellationToken.stop_requested())
			{
				return fail("Artifact publication was cancelled.");
			}
			ArtifactBundlePublishedFile& published = result.Files.emplace_back();
			if (!WriteFile(stagingDirectory, file, published, result.ErrorMessage))
			{
				return fail(result.ErrorMessage);
			}
		}
		ArtifactBundlePublishedFile& publishedCompletion = result.Files.emplace_back();
		if (!WriteFile(stagingDirectory, *completionFile, publishedCompletion, result.ErrorMessage))
		{
			return fail(result.ErrorMessage);
		}
		if (cancellationToken.stop_requested())
		{
			return fail("Artifact publication was cancelled.");
		}

		std::filesystem::rename(stagingDirectory, publicationDirectory, fileError);
		if (fileError)
		{
			return fail("Staged artifact bundle could not be published.");
		}
		ownsStagingDirectory = false;
		result.Succeeded = true;
		result.PublicationDirectory = publicationDirectory;
		return result;
	}
	catch (const std::exception& exception)
	{
		return fail(exception.what());
	}
	catch (...)
	{
		return fail("Unknown artifact publication failure.");
	}
}
