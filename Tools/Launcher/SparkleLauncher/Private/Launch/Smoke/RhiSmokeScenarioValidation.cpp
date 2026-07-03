#include "Smoke/RhiSmokeScenarioValidation.h"

#include "Smoke/RhiSmokeTestCatalog.h"

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace SparkleLauncher::RhiSmokeValidation
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
		return ImageComparisonThresholds{1.0, 0.75, false};
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

	bool ReadTextFile(const std::filesystem::path& path, std::string& outText)
	{
		std::vector<std::uint8_t> bytes;
		if (!ReadFileBytes(path, bytes))
		{
			return false;
		}
		outText.assign(bytes.begin(), bytes.end());
		return true;
	}

	bool FileExistsAndIsNotEmpty(const std::filesystem::path& path)
	{
		std::error_code errorCode;
		return std::filesystem::is_regular_file(path, errorCode) && std::filesystem::file_size(path, errorCode) > 0;
	}

	bool MetadataContains(const std::filesystem::path& metadataPath, std::string_view expectedText)
	{
		std::string metadata;
		return ReadTextFile(metadataPath, metadata) && metadata.find(expectedText) != std::string::npos;
	}

	std::string EscapeCsv(std::string_view value)
	{
		bool requiresQuotes = false;
		for (const char character : value)
		{
			if (character == '"' || character == ',' || character == '\n' || character == '\r')
			{
				requiresQuotes = true;
				break;
			}
		}
		if (!requiresQuotes)
		{
			return std::string(value);
		}

		std::string escaped;
		escaped.reserve(value.size() + 2);
		escaped.push_back('"');
		for (const char character : value)
		{
			if (character == '"')
			{
				escaped += "\"\"";
			}
			else
			{
				escaped.push_back(character);
			}
		}
		escaped.push_back('"');
		return escaped;
	}

	std::string EscapeMarkdown(std::string_view value)
	{
		std::string escaped;
		escaped.reserve(value.size());
		for (const char character : value)
		{
			if (character == '|')
			{
				escaped += "\\|";
			}
			else
			{
				escaped.push_back(character);
			}
		}
		return escaped;
	}

	bool ExtractCsvRow(const std::filesystem::path& path, std::string& outHeader, std::string& outRow)
	{
		std::ifstream file(path);
		return static_cast<bool>(file) && static_cast<bool>(std::getline(file, outHeader)) && static_cast<bool>(std::getline(file, outRow));
	}

	bool ExtractMetadataField(const std::filesystem::path& metadataPath, std::string_view key, std::string& outValue)
	{
		std::string metadata;
		if (!ReadTextFile(metadataPath, metadata))
		{
			return false;
		}

		const std::string needle = "\"" + std::string(key) + "\": \"";
		const std::size_t keyPos = metadata.find(needle);
		if (keyPos == std::string::npos)
		{
			return false;
		}

		const std::size_t valueStart = keyPos + needle.size();
		const std::size_t valueEnd = metadata.find('"', valueStart);
		if (valueEnd == std::string::npos)
		{
			return false;
		}

		outValue = metadata.substr(valueStart, valueEnd - valueStart);
		return true;
	}

	bool LogContainsFatalGraphicsIssue(const std::filesystem::path& logPath, std::string& outFailureSummary)
	{
		std::string logText;
		if (!ReadTextFile(logPath, logText))
		{
			outFailureSummary = "Missing smoke log artifact: " + logPath.string();
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
				outFailureSummary = "Fatal graphics issue found in smoke log: " + logPath.string() + " marker='" + std::string(fatalMarker) + "'";
				return true;
			}
		}

		return false;
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
		if (reference.Width != candidate.Width || reference.Height != candidate.Height || reference.BytesPerPixel != candidate.BytesPerPixel)
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
					    std::abs(static_cast<int>(reference.Bytes[referencePixel + channel]) - static_cast<int>(candidate.Bytes[candidatePixel + channel])));
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
		if (result.AverageAbsoluteDifference > thresholds.MaxAverageAbsoluteDifference || result.DifferentPixelRatio > thresholds.MaxDifferentPixelRatio)
		{
			return false;
		}
		return !thresholds.RequireExactPixels || result.DifferentPixels == 0;
	}

	bool ValidateRequiredArtifactsForSuite(const LaunchOperationPlan& plan, RhiSmokeSuite suite, std::string& outFailureSummary)
	{
		for (const RhiSmokeScenarioCase& scenarioCase : GetRhiSmokeCases(suite))
		{
			for (const RhiSmokeScenarioViewMode& viewMode : GetRhiSmokeViewModes(suite))
			{
				const std::filesystem::path bmpPath = GetRhiSmokeArtifactPath(plan, suite, scenarioCase, viewMode, ".bmp");
				const std::filesystem::path metadataPath = GetRhiSmokeArtifactPath(plan, suite, scenarioCase, viewMode, ".json");
				const std::filesystem::path timingPath = GetRhiSmokeArtifactPath(plan, suite, scenarioCase, viewMode, ".timing.csv");
				const std::filesystem::path logPath = GetRhiSmokeArtifactPath(plan, suite, scenarioCase, viewMode, ".log");
				for (const std::filesystem::path& requiredPath : {bmpPath, metadataPath, timingPath, logPath})
				{
					if (!FileExistsAndIsNotEmpty(requiredPath))
					{
						outFailureSummary = "Missing or empty smoke artifact: " + requiredPath.string();
						return false;
					}
				}
				if (LogContainsFatalGraphicsIssue(logPath, outFailureSummary))
				{
					return false;
				}

				const std::string expectedRequestedWriterPath = "\"requestedOperationWriterPath\": \"" + std::string(scenarioCase.RequestedWriterPathName) + "\"";
				if (!MetadataContains(metadataPath, expectedRequestedWriterPath))
				{
					outFailureSummary = "Smoke metadata does not report requested writer path: " + metadataPath.string();
					return false;
				}
				const std::string expectedSelectedWriterPath = "\"operationWriterPath\": \"" + std::string(scenarioCase.ExpectedSelectedWriterPathName) + "\"";
				if (!MetadataContains(metadataPath, expectedSelectedWriterPath))
				{
					outFailureSummary = "Smoke metadata does not report selected writer path: " + metadataPath.string();
					return false;
				}
				const std::string expectedWriterReason = "\"operationWriterReason\": \"" + std::string(scenarioCase.ExpectedWriterReason) + "\"";
				if (!MetadataContains(metadataPath, expectedWriterReason))
				{
					outFailureSummary = "Smoke metadata does not report expected writer reason: " + metadataPath.string();
					return false;
				}

			}
		}
		return true;
	}

	bool ValidateParityComparisons(const LaunchOperationPlan& plan, std::string& outFailureSummary)
	{
		const std::filesystem::path artifactDirectory = GetRhiSmokeArtifactDirectory(plan, RhiSmokeSuite::RayTracingParity);
		const auto validateComparison =
		    [&artifactDirectory, &outFailureSummary](
		        std::string_view referenceCase,
		        std::string_view candidateCase,
		        bool requirePartitionedTlasCandidate,
		        const ImageComparisonThresholds& thresholds) -> bool
		{
			const std::filesystem::path referencePath = artifactDirectory / referenceCase / "Lit.bmp";
			const std::filesystem::path candidatePath = artifactDirectory / candidateCase / "Lit.bmp";
			const std::filesystem::path candidateMetadataPath = artifactDirectory / candidateCase / "Lit.json";
			if (requirePartitionedTlasCandidate &&
			    !MetadataContains(candidateMetadataPath, "\"topLevelProvider\": \"PartitionedTlas\"") &&
			    !(MetadataContains(candidateMetadataPath, "\"topLevelProvider\": \"ClassicTlas\"") &&
			      MetadataContains(candidateMetadataPath, "\"ptlasSupported\": false")))
			{
				outFailureSummary = "Parity candidate did not report active PTLAS or explicit unsupported-provider fallback: " + candidateMetadataPath.string();
				return false;
			}

			ImageComparisonResult comparison;
			std::string error;
			if (!CompareBmpImages(referencePath, candidatePath, comparison, error))
			{
				outFailureSummary = error;
				return false;
			}
			if (!PassesThresholds(comparison, thresholds))
			{
				outFailureSummary = "Parity comparison failed for " + std::string(candidateCase) + " vs " + std::string(referenceCase) +
				                    ": avgAbsDiff=" + std::to_string(comparison.AverageAbsoluteDifference) +
				                    " differentPixels=" + std::to_string(comparison.DifferentPixels) +
				                    " differentPixelRatio=" + std::to_string(comparison.DifferentPixelRatio);
				return false;
			}
			return true;
		};

		return validateComparison("vulkan-classic", "vulkan-ptlas", true, ExactImageMatchThresholds()) &&
		       validateComparison("d3d12-classic", "d3d12-ptlas", true, ExactImageMatchThresholds()) &&
		       validateComparison("d3d12-classic", "vulkan-classic", false, CrossBackendBaselineThresholds());
	}

	bool WriteBenchmarkSummary(const LaunchOperationPlan& plan, std::string& outFailureSummary)
	{
		const std::filesystem::path summaryPath = GetRhiSmokeArtifactDirectory(plan, RhiSmokeSuite::PtlasBenchmark) / "benchmark-summary.csv";
		std::filesystem::create_directories(summaryPath.parent_path());

		std::ofstream file(summaryPath, std::ios::out | std::ios::trunc);
		if (!file)
		{
			outFailureSummary = "Could not create PTLAS benchmark summary CSV: " + summaryPath.string();
			return false;
		}

		bool wroteHeader = false;
		for (const RhiSmokeScenarioCase& scenarioCase : GetRhiSmokeCases(RhiSmokeSuite::PtlasBenchmark))
		{
			const RhiSmokeScenarioViewMode litViewMode{"Lit", ""};
			const std::filesystem::path timingPath = GetRhiSmokeArtifactPath(plan, RhiSmokeSuite::PtlasBenchmark, scenarioCase, litViewMode, ".timing.csv");
			const std::filesystem::path metadataPath = GetRhiSmokeArtifactPath(plan, RhiSmokeSuite::PtlasBenchmark, scenarioCase, litViewMode, ".json");

			std::string header;
			std::string row;
			if (!ExtractCsvRow(timingPath, header, row))
			{
				outFailureSummary = "Could not read PTLAS benchmark timing CSV row: " + timingPath.string();
				return false;
			}

			const bool requestedPtlas = scenarioCase.PreferPartitionedTlas;
			const bool selectedPartitionedTlas = MetadataContains(metadataPath, "\"topLevelProvider\": \"PartitionedTlas\"");
			const bool explicitFallback = requestedPtlas &&
			                              MetadataContains(metadataPath, "\"topLevelProvider\": \"ClassicTlas\"") &&
			                              MetadataContains(metadataPath, "\"ptlasSupported\": false");
			const char* artifactStatus = explicitFallback ? "SkippedWithFallbackReason" : "Captured";
			const char* skipReason = explicitFallback ? "provider-requested-but-not-supported" : "";

			if (!wroteHeader)
			{
				file << "caseName,requestedBackend,requestedTopLevelMode,requestedWriterPath,requestedPtlas,artifactStatus,skipReason," << header << '\n';
				wroteHeader = true;
			}

			file << EscapeCsv(scenarioCase.Name) << ','
			     << EscapeCsv(scenarioCase.Backend) << ','
			     << EscapeCsv(scenarioCase.RequestedTopLevelMode) << ','
			     << EscapeCsv(scenarioCase.RequestedWriterPathName) << ','
			     << (requestedPtlas ? "true" : "false") << ','
			     << EscapeCsv(artifactStatus) << ','
			     << EscapeCsv(skipReason) << ','
			     << row << '\n';

			if (requestedPtlas && !selectedPartitionedTlas && !explicitFallback)
			{
				outFailureSummary = "Benchmark case requested PTLAS but metadata did not report active PTLAS or explicit fallback: " + metadataPath.string();
				return false;
			}
		}

		return true;
	}

}

namespace SparkleLauncher
{
	bool ValidateRhiSmokeScenarioArtifacts(const LaunchOperationPlan& plan, std::string& outFailureSummary)
	{
		for (const RhiSmokeSuite suite : GetEnabledRhiSmokeSuites(plan))
		{
			if (suite == RhiSmokeSuite::SingleViewportCapture)
			{
				continue;
			}
			if (!RhiSmokeValidation::ValidateRequiredArtifactsForSuite(plan, suite, outFailureSummary))
			{
				return false;
			}
		}

		if (plan.Request.SmokeRunRayTracingParity && !RhiSmokeValidation::ValidateParityComparisons(plan, outFailureSummary))
		{
			return false;
		}
		if (plan.Request.SmokeRunPtlasBenchmark && !RhiSmokeValidation::WriteBenchmarkSummary(plan, outFailureSummary))
		{
			return false;
		}
		return true;
	}
}
