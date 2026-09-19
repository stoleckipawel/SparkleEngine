#include "PCH.h"

#include "Editor/ReferencePathTracer/ReferencePathTracerArtifactEncoding.h"

#include "Core/Public/Files/BinaryBufferWriter.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Json/JsonWriter.h"
#include "Editor/Capture/ImageEncoding.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <span>

static bool EncodeCheckpoint(
    const LinearRgbImage& mean,
    const LinearRgbImage& moment2,
    const ViewportCaptureResult& source,
    std::vector<std::byte>& checkpoint,
    std::string& errorMessage)
{
	static_assert(std::endian::native == std::endian::little);
	if (mean.Width != moment2.Width || mean.Height != moment2.Height)
	{
		errorMessage = "Checkpoint planes do not have the same extent.";
		return false;
	}

	std::array<std::vector<std::uint8_t>, 3> planes;
	const auto meanBytes = std::as_bytes(std::span<const float>(mean.Pixels.data(), mean.Pixels.size()));
	const auto moment2Bytes = std::as_bytes(std::span<const float>(moment2.Pixels.data(), moment2.Pixels.size()));
	planes[0].assign(
	    reinterpret_cast<const std::uint8_t*>(meanBytes.data()),
	    reinterpret_cast<const std::uint8_t*>(meanBytes.data() + meanBytes.size()));
	planes[1].assign(
	    reinterpret_cast<const std::uint8_t*>(moment2Bytes.data()),
	    reinterpret_cast<const std::uint8_t*>(moment2Bytes.data() + moment2Bytes.size()));
	const std::size_t pixelCount = static_cast<std::size_t>(mean.Width) * mean.Height;
	planes[2].resize(pixelCount * sizeof(std::uint32_t));
	const std::uint32_t committed = static_cast<std::uint32_t>(source.SamplePrefix.SampleCount);
	for (std::size_t pixel = 0; pixel < pixelCount; ++pixel)
	{
		std::memcpy(planes[2].data() + pixel * sizeof(committed), &committed, sizeof(committed));
	}

	std::array<Hash::Sha256Digest, 3> planeHashes{};
	for (std::size_t plane = 0; plane < planes.size(); ++plane)
	{
		if (!Hash::TrySha256(planes[plane].data(), planes[plane].size(), planeHashes[plane], errorMessage))
		{
			return false;
		}
	}

	std::vector<std::uint8_t> bytes;
	Files::BinaryBufferWriter writer(bytes);
	const std::array<std::uint8_t, 8> magic = {'S', 'P', 'K', 'R', 'P', 'T', '0', '1'};
	writer.WriteArray(std::span<const std::uint8_t>(magic));
	const std::size_t schemaBytesOffset = bytes.size();
	writer.WriteValue<std::uint32_t>(0u);
	writer.WriteValue(mean.Width);
	writer.WriteValue(mean.Height);
	writer.WriteValue<std::uint32_t>(1u);
	writer.WriteValue<std::uint32_t>(1u);
	writer.WriteValue<std::uint64_t>(0u);
	writer.WriteValue(source.SamplePrefix.SampleCount);
	writer.WriteArray(std::span<const std::byte>(source.SamplePrefix.RenderIdentitySha256));
	const std::size_t headerHashOffset = bytes.size();
	const Hash::Sha256Digest zeroHash{};
	writer.WriteArray(std::span<const std::byte>(zeroHash));
	const std::array<std::array<std::uint8_t, 8>, 3> names = {
	    std::array<std::uint8_t, 8>{'M', 'e', 'a', 'n', 'R', 'G', 'B', 0},
	    std::array<std::uint8_t, 8>{'M', '2', 'R', 'G', 'B', 0, 0, 0},
	    std::array<std::uint8_t, 8>{'C', 'o', 'u', 'n', 't', 0, 0, 0}};
	const std::uint32_t schemaBytes = static_cast<std::uint32_t>(bytes.size());
	std::memcpy(bytes.data() + schemaBytesOffset, &schemaBytes, sizeof(schemaBytes));
	Hash::Sha256Digest headerHash{};
	if (!Hash::TrySha256(bytes.data(), bytes.size(), headerHash, errorMessage))
	{
		return false;
	}
	std::memcpy(bytes.data() + headerHashOffset, headerHash.data(), headerHash.size());

	for (const std::vector<std::uint8_t>& plane : planes)
	{
		writer.WriteBytes(plane);
	}
	writer.WriteValue<std::uint32_t>(3u);
	for (std::size_t plane = 0; plane < planes.size(); ++plane)
	{
		writer.WriteArray(std::span<const std::uint8_t>(names[plane]));
		writer.WriteValue<std::uint64_t>(planes[plane].size());
		writer.WriteArray(std::span<const std::byte>(planeHashes[plane]));
	}
	checkpoint.assign(reinterpret_cast<const std::byte*>(bytes.data()), reinterpret_cast<const std::byte*>(bytes.data() + bytes.size()));
	return true;
}

bool ReferencePathTracerArtifactEncoding::Encode(
    const ReferencePathTracerArtifactWriteRequest& request,
    ReferencePathTracerEncodedArtifact& artifact,
    std::string& errorMessage)
{
	artifact = {};
	const ImageBufferView meanSource{
	    .Pixels = request.Mean.Pixels,
	    .Width = request.Mean.Width,
	    .Height = request.Mean.Height,
	    .RowPitch = request.Mean.RowPitch,
	    .Format = request.Mean.Format};
	LinearRgbImage mean;
	if (!ImageEncoding::DecodeLinearRgb(meanSource, mean, errorMessage))
	{
		return false;
	}
	artifact.Width = mean.Width;
	artifact.Height = mean.Height;
	if (!ImageEncoding::EncodeExr(mean, artifact.Beauty, errorMessage))
	{
		return false;
	}
	if (request.Kind != ReferencePathTracerArtifactKind::Checkpoint)
	{
		return true;
	}
	if (!request.Moment2)
	{
		errorMessage = "Checkpoint M2 readback is unavailable.";
		return false;
	}
	const ImageBufferView moment2Source{
	    .Pixels = request.Moment2->Pixels,
	    .Width = request.Moment2->Width,
	    .Height = request.Moment2->Height,
	    .RowPitch = request.Moment2->RowPitch,
	    .Format = request.Moment2->Format};
	LinearRgbImage moment2;
	artifact.Checkpoint.emplace();
	if (!ImageEncoding::DecodeLinearRgb(moment2Source, moment2, errorMessage)
	    || !EncodeCheckpoint(mean, moment2, request.Mean.Result, *artifact.Checkpoint, errorMessage))
	{
		artifact.Checkpoint.reset();
		return false;
	}
	return true;
}

std::string ReferencePathTracerArtifactEncoding::BuildManifest(
    const ReferencePathTracerArtifactWriteRequest& request,
    const ReferencePathTracerEncodedArtifact& artifact,
    std::string_view beautyHash,
    std::string_view checkpointHash)
{
	const char* status = request.Kind == ReferencePathTracerArtifactKind::Complete
	    ? "Complete"
	    : (request.Kind == ReferencePathTracerArtifactKind::Checkpoint ? "Checkpoint" : "PartialPrefix");
	std::vector<std::pair<std::string, std::string>> properties = {
	    {"beauty", Json::QuoteString("beauty.exr")},
	    {"beautyBytes", std::to_string(artifact.Beauty.size())},
	    {"beautySha256", Json::QuoteString(beautyHash)},
	    {"committedBegin", "0"},
	    {"committedEnd", std::to_string(request.Mean.Result.SamplePrefix.SampleCount)},
	    {"displayProduct", Json::QuoteString("separate viewport derivative")},
	    {"frameId", std::to_string(request.Mean.Result.FrameId)},
	    {"height", std::to_string(artifact.Height)},
	    {"providerGeneration", std::to_string(request.Mean.Result.ProviderGeneration)},
	    {"rawProduct", Json::QuoteString("scene-linear RGB32F")},
	    {"sceneGeneration", std::to_string(request.Mean.Result.SceneGeneration)},
	    {"schema", Json::QuoteString("sparkle-reference-path-tracer-artifact-v1")},
	    {"sessionDigestSha256", Json::QuoteString(Hash::Sha256ToHex(request.Mean.Result.SamplePrefix.RenderIdentitySha256))},
	    {"status", Json::QuoteString(status)},
	    {"targetSpp", std::to_string(request.Mean.Result.SamplePrefix.TargetSampleCount)},
	    {"width", std::to_string(artifact.Width)}};
	if (!checkpointHash.empty())
	{
		properties.emplace_back("checkpoint", Json::QuoteString("checkpoint.bin"));
		properties.emplace_back("checkpointBytes", std::to_string(artifact.Checkpoint->size()));
		properties.emplace_back("checkpointSha256", Json::QuoteString(checkpointHash));
	}
	std::sort(properties.begin(), properties.end(), [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });
	std::string manifest = "{";
	for (std::size_t index = 0; index < properties.size(); ++index)
	{
		if (index != 0u)
		{
			manifest += ',';
		}
		manifest += Json::QuoteString(properties[index].first);
		manifest += ':';
		manifest += properties[index].second;
	}
	manifest += '}';
	return manifest;
}
