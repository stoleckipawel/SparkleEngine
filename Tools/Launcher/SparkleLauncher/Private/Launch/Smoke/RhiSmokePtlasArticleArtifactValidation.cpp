#include "Smoke/RhiSmokePtlasArticleArtifactValidation.h"

#include "Smoke/RhiSmokeTestCatalog.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace SparkleLauncher::RhiSmokePtlasArticleValidation
{
	bool ReadTextFile(const std::filesystem::path& path, std::string& outText)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file)
		{
			return false;
		}

		outText.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
		return true;
	}

	bool FileExistsAndIsNotEmpty(const std::filesystem::path& path)
	{
		std::error_code errorCode;
		return std::filesystem::is_regular_file(path, errorCode) && std::filesystem::file_size(path, errorCode) > 0;
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
				continue;
			}

			escaped.push_back(character);
		}
		return escaped;
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
				continue;
			}

			escaped.push_back(character);
		}
		escaped.push_back('"');
		return escaped;
	}

	bool MetadataContains(const std::filesystem::path& metadataPath, std::string_view expectedText)
	{
		std::string metadata;
		return ReadTextFile(metadataPath, metadata) && metadata.find(expectedText) != std::string::npos;
	}

	bool ExtractPurposeValue(const std::filesystem::path& metadataPath, std::string& outPurpose)
	{
		std::string metadata;
		if (!ReadTextFile(metadataPath, metadata))
		{
			return false;
		}

		const std::string key = "\"purpose\": \"";
		const std::size_t keyPos = metadata.find(key);
		if (keyPos == std::string::npos)
		{
			return false;
		}

		const std::size_t valueStart = keyPos + key.size();
		const std::size_t valueEnd = metadata.find('"', valueStart);
		if (valueEnd == std::string::npos)
		{
			return false;
		}

		outPurpose = metadata.substr(valueStart, valueEnd - valueStart);
		return true;
	}

	bool ExtractCsvRow(const std::filesystem::path& path, std::string& outHeader, std::string& outRow)
	{
		std::ifstream file(path);
		if (!file)
		{
			return false;
		}

		return static_cast<bool>(std::getline(file, outHeader)) && static_cast<bool>(std::getline(file, outRow));
	}

	bool LogContainsFatalGraphicsIssue(const std::filesystem::path& logPath, std::string& outFailureSummary)
	{
		std::string logText;
		if (!ReadTextFile(logPath, logText))
		{
			outFailureSummary = "Missing PTLAS article log artifact: " + logPath.string();
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
				outFailureSummary = "Fatal graphics issue found in PTLAS article log: " + logPath.string() +
				                    " marker='" + std::string(fatalMarker) + "'";
				return true;
			}
		}

		return false;
	}

	bool ValidateRequiredArtifacts(const LaunchOperationPlan& plan, std::string& outFailureSummary)
	{
		for (const RhiSmokePtlasArticleCase& articleCase : GetRhiSmokePtlasArticleCases())
		{
			for (const RhiSmokePtlasArticleViewMode& viewMode : GetRhiSmokePtlasArticleViewModes())
			{
				const std::filesystem::path bmpPath = GetRhiSmokePtlasArticleArtifactPath(plan, articleCase, viewMode, ".bmp");
				const std::filesystem::path metadataPath = GetRhiSmokePtlasArticleArtifactPath(plan, articleCase, viewMode, ".json");
				const std::filesystem::path timingPath = GetRhiSmokePtlasArticleArtifactPath(plan, articleCase, viewMode, ".timing.csv");
				const std::filesystem::path logPath = GetRhiSmokePtlasArticleArtifactPath(plan, articleCase, viewMode, ".log");
				for (const std::filesystem::path& requiredPath : {bmpPath, metadataPath, timingPath, logPath})
				{
					if (!FileExistsAndIsNotEmpty(requiredPath))
					{
						outFailureSummary = "Missing or empty PTLAS article artifact: " + requiredPath.string();
						return false;
					}
				}

				if (LogContainsFatalGraphicsIssue(logPath, outFailureSummary))
				{
					return false;
				}

				const std::string expectedPurpose = "\"purpose\": \"" + std::string(viewMode.Purpose) + "\"";
				const std::string expectedStoryLabel = "\"storyLabel\": \"" + std::string(articleCase.StoryLabel) + "\"";
				if (!MetadataContains(metadataPath, expectedPurpose))
				{
					outFailureSummary = "PTLAS article metadata does not report capture purpose: " + metadataPath.string();
					return false;
				}
				if (!MetadataContains(metadataPath, expectedStoryLabel))
				{
					outFailureSummary = "PTLAS article metadata does not report story label: " + metadataPath.string();
					return false;
				}
			}
		}

		return true;
	}

	bool WriteCaptureIndex(const LaunchOperationPlan& plan, std::string& outFailureSummary)
	{
		const std::filesystem::path articleDirectory = GetRhiSmokePtlasArticleArtifactDirectory(plan);
		const std::filesystem::path indexPath = articleDirectory / "capture-index.md";
		std::ofstream file(indexPath, std::ios::out | std::ios::trunc);
		if (!file)
		{
			outFailureSummary = "Could not create PTLAS article capture index: " + indexPath.string();
			return false;
		}

		file << "# PTLAS Article Capture Pack\n\n";
		file << "This folder was generated by launcher workflow `project.run.rhi-raytracing-ptlas-article`.\n\n";
		file << "## Cases\n\n";
		file << "| Case | Backend | Requested top-level mode | Requested writer path | Story label |\n";
		file << "|---|---|---|---|---|\n";
		for (const RhiSmokePtlasArticleCase& articleCase : GetRhiSmokePtlasArticleCases())
		{
			file << "| " << EscapeMarkdown(articleCase.Name)
			     << " | " << EscapeMarkdown(articleCase.Backend)
			     << " | " << EscapeMarkdown(articleCase.RequestedTopLevelMode)
			     << " | " << EscapeMarkdown(articleCase.RequestedWriterPathName)
			     << " | " << EscapeMarkdown(articleCase.StoryLabel) << " |\n";
		}

		file << "\n## Screenshots\n\n";
		file << "| Case | View mode | Purpose | Screenshot | Metadata | Timing CSV |\n";
		file << "|---|---|---|---|---|---|\n";
		for (const RhiSmokePtlasArticleCase& articleCase : GetRhiSmokePtlasArticleCases())
		{
			for (const RhiSmokePtlasArticleViewMode& viewMode : GetRhiSmokePtlasArticleViewModes())
			{
				const std::filesystem::path bmpPath = GetRhiSmokePtlasArticleArtifactPath(plan, articleCase, viewMode, ".bmp");
				const std::filesystem::path metadataPath = GetRhiSmokePtlasArticleArtifactPath(plan, articleCase, viewMode, ".json");
				const std::filesystem::path timingPath = GetRhiSmokePtlasArticleArtifactPath(plan, articleCase, viewMode, ".timing.csv");

				file << "| " << EscapeMarkdown(articleCase.Name)
				     << " | " << EscapeMarkdown(viewMode.Name)
				     << " | " << EscapeMarkdown(viewMode.Purpose)
				     << " | [" << EscapeMarkdown(bmpPath.filename().string()) << "](" << articleCase.Name << "/" << viewMode.Name << ".bmp)"
				     << " | [" << EscapeMarkdown(metadataPath.filename().string()) << "](" << articleCase.Name << "/" << viewMode.Name << ".json)"
				     << " | [" << EscapeMarkdown(timingPath.filename().string()) << "](" << articleCase.Name << "/" << viewMode.Name << ".timing.csv)"
				     << " |\n";
			}
		}

		file << "\n## External Capture Notes\n\n";
		file << "### Expected GPU marker names\n\n";
		file << "- `GPU Frame`\n";
		file << "- `RayTracing.BLAS.Build`\n";
		file << "- `RayTracing.TLAS.Classic.Build`\n";
		file << "- `RayTracing.PTLAS.LogicalDirty`\n";
		file << "- `RayTracing.PTLAS.NativePack`\n";
		file << "- `RayTracing.PTLAS.Update`\n";
		file << "- `RayTracing.TraceOrRayQuery`\n";
		file << "- `Partitioned TLAS Build`\n\n";
		file << "### Suggested capture frame\n\n";
		file << "- Capture around frame `80`, which is the smoke screenshot frame used for this pack.\n";
		file << "- Camera motion runs from frame `10` through frame `70`, so frame `80` should show settled results after deterministic motion.\n\n";
		file << "### What to inspect in Nsight Graphics or PIX\n\n";
		file << "- Confirm whether top-level AS selection is `ClassicTlas` or `PartitionedTlas` in metadata and provider status view.\n";
		file << "- Inspect top-level acceleration structure bindings and confirm fallback reasons when PTLAS is not active.\n";
		file << "- Correlate `RayTracing.PTLAS.*` markers with timing CSV rows for native operation pressure and PTLAS update cost.\n";
		file << "- Use `RayTracingPartitions`, `RayTracingPartitionUpdates`, and `RayTracingNativeOperations` screenshots as the visual explanation layer for the captured frame.\n";
		return true;
	}

	bool WriteCaptureSummaryCsv(const LaunchOperationPlan& plan, std::string& outFailureSummary)
	{
		const std::filesystem::path summaryPath = GetRhiSmokePtlasArticleArtifactDirectory(plan) / "capture-summary.csv";
		std::ofstream file(summaryPath, std::ios::out | std::ios::trunc);
		if (!file)
		{
			outFailureSummary = "Could not create PTLAS article capture summary CSV: " + summaryPath.string();
			return false;
		}

		file << "caseName,backend,requestedTopLevelMode,requestedWriterPath,viewMode,purpose,artifactStatus,topLevelProvider,ptlasProvider,"
		        "ptlasSupported,operationWriterPath,operationWriterReason,metadataPath,timingCsvPath,screenshotPath\n";

		for (const RhiSmokePtlasArticleCase& articleCase : GetRhiSmokePtlasArticleCases())
		{
			for (const RhiSmokePtlasArticleViewMode& viewMode : GetRhiSmokePtlasArticleViewModes())
			{
				const std::filesystem::path metadataPath = GetRhiSmokePtlasArticleArtifactPath(plan, articleCase, viewMode, ".json");
				const std::filesystem::path timingPath = GetRhiSmokePtlasArticleArtifactPath(plan, articleCase, viewMode, ".timing.csv");
				const std::filesystem::path bmpPath = GetRhiSmokePtlasArticleArtifactPath(plan, articleCase, viewMode, ".bmp");
				const bool selectedPartitionedTlas = MetadataContains(metadataPath, "\"topLevelProvider\": \"PartitionedTlas\"");
				const bool explicitFallback = MetadataContains(metadataPath, "\"topLevelProvider\": \"ClassicTlas\"") &&
				                              MetadataContains(metadataPath, "\"ptlasSupported\": false");
				const char* artifactStatus = explicitFallback ? "CapturedWithFallback" : "Captured";

				std::string purpose;
				if (!ExtractPurposeValue(metadataPath, purpose))
				{
					purpose = viewMode.Purpose;
				}

				std::string metadataText;
				ReadTextFile(metadataPath, metadataText);

				auto extractField = [&metadataText](std::string_view key, std::string_view fallback) -> std::string
				{
					const std::string needle = "\"" + std::string(key) + "\": \"";
					const std::size_t keyPos = metadataText.find(needle);
					if (keyPos == std::string::npos)
					{
						return std::string(fallback);
					}
					const std::size_t valueStart = keyPos + needle.size();
					const std::size_t valueEnd = metadataText.find('"', valueStart);
					return valueEnd == std::string::npos ? std::string(fallback) : metadataText.substr(valueStart, valueEnd - valueStart);
				};

				file << EscapeCsv(articleCase.Name) << ','
				     << EscapeCsv(articleCase.Backend) << ','
				     << EscapeCsv(articleCase.RequestedTopLevelMode) << ','
				     << EscapeCsv(articleCase.RequestedWriterPathName) << ','
				     << EscapeCsv(viewMode.Name) << ','
				     << EscapeCsv(purpose) << ','
				     << EscapeCsv(artifactStatus) << ','
				     << EscapeCsv(extractField("topLevelProvider", "Unknown")) << ','
				     << EscapeCsv(extractField("ptlasProvider", "Unknown")) << ','
				     << EscapeCsv(MetadataContains(metadataPath, "\"ptlasSupported\": true") ? "true" : "false") << ','
				     << EscapeCsv(extractField("operationWriterPath", "Unknown")) << ','
				     << EscapeCsv(extractField("operationWriterReason", "unknown")) << ','
				     << EscapeCsv(metadataPath.string()) << ','
				     << EscapeCsv(timingPath.string()) << ','
				     << EscapeCsv(bmpPath.string()) << '\n';

				if (articleCase.PreferPartitionedTlas && !selectedPartitionedTlas && !explicitFallback)
				{
					outFailureSummary = "PTLAS article case requested PTLAS but metadata did not report active PTLAS or explicit fallback: " +
					                    metadataPath.string();
					return false;
				}
			}
		}

		return true;
	}
}

namespace SparkleLauncher
{
	bool ValidateRhiSmokePtlasArticleArtifacts(const LaunchOperationPlan& plan, std::string& outFailureSummary)
	{
		return RhiSmokePtlasArticleValidation::ValidateRequiredArtifacts(plan, outFailureSummary) &&
		       RhiSmokePtlasArticleValidation::WriteCaptureIndex(plan, outFailureSummary) &&
		       RhiSmokePtlasArticleValidation::WriteCaptureSummaryCsv(plan, outFailureSummary);
	}
}
