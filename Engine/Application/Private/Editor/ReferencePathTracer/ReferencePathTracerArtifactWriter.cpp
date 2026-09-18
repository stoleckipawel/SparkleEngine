#include "PCH.h"

#include "Editor/ReferencePathTracer/ReferencePathTracerArtifactWriter.h"

#include "Core/Public/Hash/HashUtils.h"
#include "Editor/Artifacts/ArtifactBundlePublication.h"
#include "Editor/ReferencePathTracer/ReferencePathTracerArtifactEncoding.h"

#include <utility>

ReferencePathTracerArtifactWriteResult ReferencePathTracerArtifactWriter::Write(
    ReferencePathTracerArtifactWriteRequest request,
    std::stop_token cancellationToken) noexcept
{
	ReferencePathTracerArtifactWriteResult result{.PublicationDirectory = request.PublicationDirectory};
	ReferencePathTracerEncodedArtifact artifact;
	if (!ReferencePathTracerArtifactEncoding::Encode(request, artifact, result.ErrorMessage))
	{
		return result;
	}

	std::string beautyHash;
	std::string checkpointHash;
	if (!Hash::TrySha256Hex(artifact.Beauty, beautyHash, result.ErrorMessage)
	    || (artifact.Checkpoint && !Hash::TrySha256Hex(*artifact.Checkpoint, checkpointHash, result.ErrorMessage)))
	{
		return result;
	}
	const std::string manifest = ReferencePathTracerArtifactEncoding::BuildManifest(request, artifact, beautyHash, checkpointHash);
	ArtifactBundlePublicationRequest publication{
	    .AllowedRoot = std::move(request.AllowedRoot),
	    .PublicationDirectory = std::move(request.PublicationDirectory),
	    .CompletionFile = "manifest.json",
	    .MaximumOutputBytes = request.MaximumOutputBytes};
	publication.Files.push_back(ArtifactBundleFile{.RelativePath = "beauty.exr", .Bytes = std::move(artifact.Beauty)});
	if (artifact.Checkpoint)
	{
		publication.Files.push_back(ArtifactBundleFile{.RelativePath = "checkpoint.bin", .Bytes = std::move(*artifact.Checkpoint)});
	}
	publication.Files.push_back(
	    ArtifactBundleFile{
	        .RelativePath = "manifest.json",
	        .Bytes = std::vector<std::byte>(
	            reinterpret_cast<const std::byte*>(manifest.data()),
	            reinterpret_cast<const std::byte*>(manifest.data() + manifest.size()))});

	ArtifactBundlePublicationResult publicationResult = ArtifactBundlePublication::Publish(std::move(publication), cancellationToken);
	result.Succeeded = publicationResult.Succeeded;
	result.PublicationDirectory = std::move(publicationResult.PublicationDirectory);
	result.ErrorMessage = std::move(publicationResult.ErrorMessage);
	if (result.Succeeded && !publicationResult.Files.empty())
	{
		result.ManifestSha256 = std::move(publicationResult.Files.back().Sha256);
	}
	return result;
}
