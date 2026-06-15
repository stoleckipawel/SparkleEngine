#include "Smoke/RhiSmokeParityArtifactValidation.h"

#include "Smoke/RhiSmokeTestCatalog.h"

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace SparkleLauncher::RhiSmokeBitmapComparison
{
	struct BmpImage final
	{
		std::vector<std::uint8_t> Bytes;
		std::int32_t Width = 0;
		std::int32_t Height = 0;
		std::int32_t RowStride = 0;
		std::int32_t BytesPerPixel = 0;
		std::int32_t PixelOffset = 0;
	};

	struct ImageComparisonResult final
	{
		double AverageAbsoluteDifference = 0.0;
		std::uint64_t DifferentPixels = 0;
		double DifferentPixelRatio = 0.0;
	};

	struct ImageComparisonThresholds final
	{
		double MaxAverageAbsoluteDifference = 0.0;
		double MaxDifferentPixelRatio = 0.0;
		bool RequireExactPixels = true;
	};

	constexpr ImageComparisonThresholds ExactImageMatchThresholds() noexcept
	{
		return ImageComparisonThresholds{};
	}

	constexpr ImageComparisonThresholds CrossBackendBaselineThresholds() noexcept
	{
		return ImageComparisonThresholds{
		    .MaxAverageAbsoluteDifference = 1.0,
		    .MaxDifferentPixelRatio = 0.75,
		    .RequireExactPixels = false};
	}

	bool ReadFileBytes(const std::filesystem::path& path, std::vector<std::uint8_t>& outBytes)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file)
		{
			return false;
		}

		outBytes.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
		return true;
	}

	std::int32_t ReadInt32(const std::vector<std::uint8_t>& bytes, std::size_t offset)
	{
		return static_cast<std::int32_t>(
		    static_cast<std::uint32_t>(bytes[offset]) |
		    (static_cast<std::uint32_t>(bytes[offset + 1]) << 8u) |
		    (static_cast<std::uint32_t>(bytes[offset + 2]) << 16u) |
		    (static_cast<std::uint32_t>(bytes[offset + 3]) << 24u));
	}

	std::int16_t ReadInt16(const std::vector<std::uint8_t>& bytes, std::size_t offset)
	{
		return static_cast<std::int16_t>(
		    static_cast<std::uint16_t>(bytes[offset]) |
		    (static_cast<std::uint16_t>(bytes[offset + 1]) << 8u));
	}

	bool LoadBmpImage(const std::filesystem::path& path, BmpImage& outImage, std::string& outError)
	{
		if (!ReadFileBytes(path, outImage.Bytes))
		{
			outError = "Missing BMP capture: " + path.string();
			return false;
		}
		if (outImage.Bytes.size() < 54 || outImage.Bytes[0] != 0x42 || outImage.Bytes[1] != 0x4D)
		{
			outError = "Unsupported BMP capture: " + path.string();
			return false;
		}

		outImage.PixelOffset = ReadInt32(outImage.Bytes, 10);
		outImage.Width = ReadInt32(outImage.Bytes, 18);
		outImage.Height = std::abs(ReadInt32(outImage.Bytes, 22));
		const std::int16_t bitsPerPixel = ReadInt16(outImage.Bytes, 28);
		if (bitsPerPixel != 24 && bitsPerPixel != 32)
		{
			outError = "Unsupported BMP bit depth in capture: " + path.string();
			return false;
		}

		outImage.BytesPerPixel = bitsPerPixel / 8;
		outImage.RowStride = ((bitsPerPixel * outImage.Width + 31) / 32) * 4;
		return true;
	}

	bool CompareBmpImages(
	    const std::filesystem::path& referencePath,
	    const std::filesystem::path& candidatePath,
	    ImageComparisonResult& outResult,
	    std::string& outError)
	{
		BmpImage reference;
		BmpImage candidate;
		if (!LoadBmpImage(referencePath, reference, outError) || !LoadBmpImage(candidatePath, candidate, outError))
		{
			return false;
		}
		if (reference.Width != candidate.Width || reference.Height != candidate.Height ||
		    reference.BytesPerPixel != candidate.BytesPerPixel)
		{
			outError = "Capture dimensions or formats differ: " + referencePath.string() + " vs " + candidatePath.string();
			return false;
		}

		std::uint64_t totalDifference = 0;
		std::uint64_t differentPixels = 0;
		const std::uint64_t pixelCount = static_cast<std::uint64_t>(reference.Width) * static_cast<std::uint64_t>(reference.Height);
		for (std::int32_t y = 0; y < reference.Height; ++y)
		{
			const std::int32_t referenceRow = reference.PixelOffset + y * reference.RowStride;
			const std::int32_t candidateRow = candidate.PixelOffset + y * candidate.RowStride;
			for (std::int32_t x = 0; x < reference.Width; ++x)
			{
				const std::int32_t referencePixel = referenceRow + x * reference.BytesPerPixel;
				const std::int32_t candidatePixel = candidateRow + x * candidate.BytesPerPixel;
				std::uint32_t pixelDifference = 0;
				for (std::int32_t channel = 0; channel < 3; ++channel)
				{
					pixelDifference += static_cast<std::uint32_t>(
					    std::abs(static_cast<int>(reference.Bytes[referencePixel + channel]) -
					             static_cast<int>(candidate.Bytes[candidatePixel + channel])));
				}
				differentPixels += pixelDifference == 0 ? 0u : 1u;
				totalDifference += pixelDifference;
			}
		}

		outResult.AverageAbsoluteDifference = static_cast<double>(totalDifference) / static_cast<double>(pixelCount * 3u);
		outResult.DifferentPixels = differentPixels;
		outResult.DifferentPixelRatio = pixelCount > 0 ? static_cast<double>(differentPixels) / static_cast<double>(pixelCount) : 1.0;
		return true;
	}

	bool PassesThresholds(const ImageComparisonResult& result, const ImageComparisonThresholds& thresholds) noexcept
	{
		if (result.AverageAbsoluteDifference > thresholds.MaxAverageAbsoluteDifference ||
		    result.DifferentPixelRatio > thresholds.MaxDifferentPixelRatio)
		{
			return false;
		}
		return !thresholds.RequireExactPixels || result.DifferentPixels == 0;
	}
}

namespace SparkleLauncher::RhiSmokeProviderMetadataValidation
{
	bool ReadTextFile(const std::filesystem::path& path, std::string& outText)
	{
		std::vector<std::uint8_t> bytes;
		if (!RhiSmokeBitmapComparison::ReadFileBytes(path, bytes))
		{
			return false;
		}

		outText.assign(bytes.begin(), bytes.end());
		return true;
	}

	bool MetadataSelectsPartitionedTlas(const std::filesystem::path& metadataPath)
	{
		std::string metadata;
		if (!ReadTextFile(metadataPath, metadata))
		{
			return false;
		}

		return metadata.find("\"topLevelProvider\": \"PartitionedTlas\"") != std::string::npos;
	}

	bool MetadataReportsUnsupportedPartitionedTlasFallback(const std::filesystem::path& metadataPath)
	{
		std::string metadata;
		if (!ReadTextFile(metadataPath, metadata))
		{
			return false;
		}

		return metadata.find("\"topLevelProvider\": \"ClassicTlas\"") != std::string::npos &&
		       metadata.find("\"ptlasSupported\": false") != std::string::npos;
	}

	bool MetadataReportsRequestedWriterPath(
	    const std::filesystem::path& metadataPath,
	    std::string_view expectedWriterPathName)
	{
		std::string metadata;
		if (!ReadTextFile(metadataPath, metadata))
		{
			return false;
		}

		const std::string expected =
		    "\"requestedOperationWriterPath\": \"" + std::string(expectedWriterPathName) + "\"";
		return metadata.find(expected) != std::string::npos;
	}

	bool MetadataReportsSelectedWriterPath(
	    const std::filesystem::path& metadataPath,
	    std::string_view expectedWriterPathName)
	{
		std::string metadata;
		if (!ReadTextFile(metadataPath, metadata))
		{
			return false;
		}

		const std::string expected =
		    "\"operationWriterPath\": \"" + std::string(expectedWriterPathName) + "\"";
		return metadata.find(expected) != std::string::npos;
	}

	bool MetadataReportsWriterReason(
	    const std::filesystem::path& metadataPath,
	    std::string_view expectedWriterReason)
	{
		std::string metadata;
		if (!ReadTextFile(metadataPath, metadata))
		{
			return false;
		}

		const std::string expected = "\"operationWriterReason\": \"" + std::string(expectedWriterReason) + "\"";
		return metadata.find(expected) != std::string::npos;
	}
}

namespace SparkleLauncher::RhiSmokeParityValidation
{
	bool FileExistsAndIsNotEmpty(const std::filesystem::path& path)
	{
		std::error_code errorCode;
		return std::filesystem::is_regular_file(path, errorCode) && std::filesystem::file_size(path, errorCode) > 0;
	}

	bool LogContainsFatalGraphicsIssue(const std::filesystem::path& logPath, std::string& outFailureSummary)
	{
		std::string logText;
		if (!RhiSmokeProviderMetadataValidation::ReadTextFile(logPath, logText))
		{
			outFailureSummary = "Missing log artifact: " + logPath.string();
			return true;
		}

		const std::vector<std::string_view> fatalMarkers = {
		    "[error]",
		    "VUID",
		    "descriptorType mismatch",
		    "invalid or has been destroyed",
		    "vkCreateComputePipelines():"};
		for (std::string_view fatalMarker : fatalMarkers)
		{
			if (logText.find(fatalMarker) != std::string::npos)
			{
				outFailureSummary = "Fatal graphics issue found in smoke artifact log: " + logPath.string() +
				                    " marker='" + std::string(fatalMarker) + "'";
				return true;
			}
		}

		return false;
	}

	bool ValidateRequiredArtifacts(const LaunchOperationPlan& plan, std::string& outFailureSummary)
	{
		for (const RhiSmokeParityCase& parityCase : GetRhiSmokeParityCases())
		{
			for (const RhiSmokeParityViewMode& viewMode : GetRhiSmokeParityViewModes())
			{
				const std::filesystem::path bmpPath = GetRhiSmokeParityArtifactPath(plan, parityCase, viewMode, ".bmp");
				const std::filesystem::path metadataPath = GetRhiSmokeParityArtifactPath(plan, parityCase, viewMode, ".json");
				const std::filesystem::path timingPath = GetRhiSmokeParityArtifactPath(plan, parityCase, viewMode, ".timing.csv");
				const std::filesystem::path logPath = GetRhiSmokeParityArtifactPath(plan, parityCase, viewMode, ".log");
				for (const std::filesystem::path& requiredPath : {bmpPath, metadataPath, timingPath, logPath})
				{
					if (!FileExistsAndIsNotEmpty(requiredPath))
					{
						outFailureSummary = "Missing or empty RHI ray tracing parity artifact: " + requiredPath.string();
						return false;
					}
				}
				if (LogContainsFatalGraphicsIssue(logPath, outFailureSummary))
				{
					return false;
				}
				if (!RhiSmokeProviderMetadataValidation::MetadataReportsRequestedWriterPath(
				        metadataPath,
				        parityCase.ExpectedRequestedWriterPathName))
				{
					outFailureSummary = "RHI ray tracing parity metadata does not report requested PTLAS writer path: " +
					                    metadataPath.string();
					return false;
				}
				if (!RhiSmokeProviderMetadataValidation::MetadataReportsSelectedWriterPath(
				        metadataPath,
				        parityCase.ExpectedSelectedWriterPathName))
				{
					outFailureSummary = "RHI ray tracing parity metadata does not report selected PTLAS writer path: " +
					                    metadataPath.string();
					return false;
				}
				if (!RhiSmokeProviderMetadataValidation::MetadataReportsWriterReason(
				        metadataPath,
				        parityCase.ExpectedWriterReason))
				{
					outFailureSummary = "RHI ray tracing parity metadata does not report expected PTLAS writer reason: " +
					                    metadataPath.string();
					return false;
				}
			}
		}
		return true;
	}

	bool ValidateComparison(
	    const LaunchOperationPlan& plan,
	    std::string_view referenceCase,
	    std::string_view candidateCase,
	    bool requirePartitionedTlasCandidate,
	    const RhiSmokeBitmapComparison::ImageComparisonThresholds& thresholds,
	    std::string& outFailureSummary)
	{
		const std::filesystem::path artifactDirectory = GetRhiSmokeParityArtifactDirectory(plan);
		const std::filesystem::path referencePath = artifactDirectory / referenceCase / "Lit.bmp";
		const std::filesystem::path candidatePath = artifactDirectory / candidateCase / "Lit.bmp";
		const std::filesystem::path candidateMetadataPath = artifactDirectory / candidateCase / "Lit.json";
		if (requirePartitionedTlasCandidate &&
		    !RhiSmokeProviderMetadataValidation::MetadataSelectsPartitionedTlas(candidateMetadataPath))
		{
			if (!RhiSmokeProviderMetadataValidation::MetadataReportsUnsupportedPartitionedTlasFallback(candidateMetadataPath))
			{
				outFailureSummary = "RHI ray tracing parity candidate did not select PTLAS and did not report an explicit "
				                    "unsupported-provider fallback: " +
				                    candidateMetadataPath.string();
				return false;
			}
		}

		RhiSmokeBitmapComparison::ImageComparisonResult comparison;
		std::string error;
		if (!RhiSmokeBitmapComparison::CompareBmpImages(referencePath, candidatePath, comparison, error))
		{
			outFailureSummary = error;
			return false;
		}
		if (!RhiSmokeBitmapComparison::PassesThresholds(comparison, thresholds))
		{
			outFailureSummary = "RHI ray tracing parity failed for " + std::string(candidateCase) +
			                    " vs " + std::string(referenceCase) + ": avgAbsDiff=" +
			                    std::to_string(comparison.AverageAbsoluteDifference) +
			                    " differentPixels=" + std::to_string(comparison.DifferentPixels) +
			                    " differentPixelRatio=" + std::to_string(comparison.DifferentPixelRatio);
			return false;
		}
		return true;
	}
}

namespace SparkleLauncher
{
	bool ValidateRhiSmokeRayTracingParityArtifacts(const LaunchOperationPlan& plan, std::string& outFailureSummary)
	{
		if (!RhiSmokeParityValidation::ValidateRequiredArtifacts(plan, outFailureSummary))
		{
			return false;
		}

		return RhiSmokeParityValidation::ValidateComparison(
		           plan,
		           "vulkan-classic",
		           "vulkan-ptlas",
		           true,
		           RhiSmokeBitmapComparison::ExactImageMatchThresholds(),
		           outFailureSummary) &&
		       RhiSmokeParityValidation::ValidateComparison(
		           plan,
		           "d3d12-classic",
		           "d3d12-ptlas",
		           true,
		           RhiSmokeBitmapComparison::ExactImageMatchThresholds(),
		           outFailureSummary) &&
		       RhiSmokeParityValidation::ValidateComparison(
		           plan,
		           "d3d12-classic",
		           "vulkan-classic",
		           false,
		           RhiSmokeBitmapComparison::CrossBackendBaselineThresholds(),
		           outFailureSummary);
	}
}
