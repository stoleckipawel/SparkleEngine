#include "Smoke/RhiSmokePtlasBenchmarkArtifactValidation.h"

#include "Smoke/RhiSmokeTestCatalog.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace SparkleLauncher::RhiSmokePtlasBenchmarkValidation
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
			outFailureSummary = "Missing PTLAS benchmark log artifact: " + logPath.string();
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
				outFailureSummary = "Fatal graphics issue found in PTLAS benchmark log: " + logPath.string() +
				                    " marker='" + std::string(fatalMarker) + "'";
				return true;
			}
		}

		return false;
	}

	bool MetadataContains(const std::filesystem::path& metadataPath, std::string_view expectedText)
	{
		std::string metadata;
		return ReadTextFile(metadataPath, metadata) && metadata.find(expectedText) != std::string::npos;
	}

	bool ValidateRequiredArtifacts(const LaunchOperationPlan& plan, std::string& outFailureSummary)
	{
		for (const RhiSmokePtlasBenchmarkCase& benchmarkCase : GetRhiSmokePtlasBenchmarkCases())
		{
			for (const RhiSmokePtlasBenchmarkViewMode& viewMode : GetRhiSmokePtlasBenchmarkViewModes())
			{
				const std::filesystem::path bmpPath = GetRhiSmokePtlasBenchmarkArtifactPath(plan, benchmarkCase, viewMode, ".bmp");
				const std::filesystem::path metadataPath = GetRhiSmokePtlasBenchmarkArtifactPath(plan, benchmarkCase, viewMode, ".json");
				const std::filesystem::path timingPath = GetRhiSmokePtlasBenchmarkArtifactPath(plan, benchmarkCase, viewMode, ".timing.csv");
				const std::filesystem::path logPath = GetRhiSmokePtlasBenchmarkArtifactPath(plan, benchmarkCase, viewMode, ".log");
				for (const std::filesystem::path& requiredPath : {bmpPath, metadataPath, timingPath, logPath})
				{
					if (!FileExistsAndIsNotEmpty(requiredPath))
					{
						outFailureSummary = "Missing or empty PTLAS benchmark artifact: " + requiredPath.string();
						return false;
					}
				}

				if (LogContainsFatalGraphicsIssue(logPath, outFailureSummary))
				{
					return false;
				}

				const std::string expectedWriterPath =
				    "\"requestedOperationWriterPath\": \"" + std::string(benchmarkCase.RequestedWriterPathName) + "\"";
				if (!MetadataContains(metadataPath, expectedWriterPath))
				{
					outFailureSummary = "PTLAS benchmark metadata does not report requested writer path: " + metadataPath.string();
					return false;
				}
			}
		}

		return true;
	}

	bool WriteSummaryCsv(const LaunchOperationPlan& plan, std::string& outFailureSummary)
	{
		const std::filesystem::path summaryPath = GetRhiSmokePtlasBenchmarkArtifactDirectory(plan) / "benchmark-summary.csv";
		std::filesystem::create_directories(summaryPath.parent_path());

		std::ofstream file(summaryPath, std::ios::out | std::ios::trunc);
		if (!file)
		{
			outFailureSummary = "Could not create PTLAS benchmark summary CSV: " + summaryPath.string();
			return false;
		}

		bool wroteHeader = false;
		for (const RhiSmokePtlasBenchmarkCase& benchmarkCase : GetRhiSmokePtlasBenchmarkCases())
		{
			const std::filesystem::path litTimingPath =
			    GetRhiSmokePtlasBenchmarkArtifactPath(plan, benchmarkCase, RhiSmokePtlasBenchmarkViewMode{"Lit"}, ".timing.csv");
			const std::filesystem::path litMetadataPath =
			    GetRhiSmokePtlasBenchmarkArtifactPath(plan, benchmarkCase, RhiSmokePtlasBenchmarkViewMode{"Lit"}, ".json");

			std::string header;
			std::string row;
			if (!ExtractCsvRow(litTimingPath, header, row))
			{
				outFailureSummary = "Could not read PTLAS benchmark timing CSV row: " + litTimingPath.string();
				return false;
			}

			bool requestedPtlas = benchmarkCase.PreferPartitionedTlas;
			const bool selectedPartitionedTlas = MetadataContains(litMetadataPath, "\"topLevelProvider\": \"PartitionedTlas\"");
			const bool explicitFallback = requestedPtlas &&
			                              MetadataContains(litMetadataPath, "\"topLevelProvider\": \"ClassicTlas\"") &&
			                              MetadataContains(litMetadataPath, "\"ptlasSupported\": false");
			const char* artifactStatus = explicitFallback ? "SkippedWithFallbackReason" : "Captured";
			const char* skipReason = explicitFallback ? "provider-requested-but-not-supported" : "";

			if (!wroteHeader)
			{
				file << "caseName,requestedBackend,requestedTopLevelMode,requestedWriterPath,requestedPtlas,artifactStatus,skipReason,"
				     << header << '\n';
				wroteHeader = true;
			}

			file << EscapeCsv(benchmarkCase.Name) << ','
			     << EscapeCsv(benchmarkCase.Backend) << ','
			     << EscapeCsv(benchmarkCase.RequestedTopLevelMode) << ','
			     << EscapeCsv(benchmarkCase.RequestedWriterPathName) << ','
			     << (requestedPtlas ? "true" : "false") << ','
			     << EscapeCsv(artifactStatus) << ','
			     << EscapeCsv(skipReason) << ','
			     << row << '\n';

			if (requestedPtlas && !selectedPartitionedTlas && !explicitFallback)
			{
				outFailureSummary = "PTLAS benchmark case requested PTLAS but metadata did not report active PTLAS or explicit fallback: " +
				                    litMetadataPath.string();
				return false;
			}
		}

		return true;
	}
}

namespace SparkleLauncher
{
	bool ValidateRhiSmokePtlasBenchmarkArtifacts(const LaunchOperationPlan& plan, std::string& outFailureSummary)
	{
		return RhiSmokePtlasBenchmarkValidation::ValidateRequiredArtifacts(plan, outFailureSummary) &&
		       RhiSmokePtlasBenchmarkValidation::WriteSummaryCsv(plan, outFailureSummary);
	}
}
