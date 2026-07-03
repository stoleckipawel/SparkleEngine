#include "PCH.h"

#include "Cooking/TextureCookRequestBatchProcessor.h"

#include "Constants/TextureCookerConstants.h"
#include "Cooking/TextureAssetCooker.h"

#include "Core/Public/Formatting/HexFormat.h"

#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Json/JsonWriter.h"
#include "ToolConsole.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <objbase.h>
#include <sstream>

static std::uint64_t TextureCookerElapsedMilliseconds(std::chrono::steady_clock::time_point startTime) noexcept
{
	return static_cast<std::uint64_t>(
	    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count());
}

static std::string TextureCookerGetRequestDisplayName(const TextureCookRequest& request)
{
	return ToolConsole::PathDisplayName(request.sourcePath);
}

	TextureCookRequestBatchProcessor::ScopedComInitializer::~ScopedComInitializer()
	{
		if (SUCCEEDED(m_result))
		{
			CoUninitialize();
		}
	}

	bool TextureCookRequestBatchProcessor::ScopedComInitializer::TryInitialize(std::string& outErrorMessage)
	{
		m_result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		if (FAILED(m_result) && m_result != RPC_E_CHANGED_MODE)
		{
			outErrorMessage = "failed to initialize COM for source texture loading";
			return false;
		}

		outErrorMessage.clear();
		return true;
	}

	int TextureCookRequestBatchProcessor::CookRequestFile(
		const std::filesystem::path& requestFilePath,
		const std::filesystem::path& summaryPath) const
	{
		const auto batchStartTime = std::chrono::steady_clock::now();

		std::string errorMessage;
		ScopedComInitializer comInitializer;
		if (!comInitializer.TryInitialize(errorMessage))
		{
			ToolConsole::Error(errorMessage);
			return TextureCookerConstants::ExitComInitializationFailed;
		}

		std::vector<TextureCookRequest> requests;
		if (!TryLoadRequests(requestFilePath, requests, errorMessage))
		{
			ToolConsole::Message(
			    std::cerr,
			    ToolConsoleSeverity::Error,
			    "Failed to load texture request file",
			    {ToolConsole::PathField("requestFile", requestFilePath), ToolConsole::QuotedField("reason", errorMessage)});
			return TextureCookerConstants::ExitLoadRequestFileFailed;
		}

		TextureAssetCooker cooker;
		std::size_t cookedCount = 0;
		std::vector<TextureCookRequestTiming> requestTimings;
		requestTimings.reserve(requests.size());
		for (std::size_t requestIndex = 0; requestIndex < requests.size(); ++requestIndex)
		{
			const TextureCookRequest& request = requests[requestIndex];
			const auto requestStartTime = std::chrono::steady_clock::now();
			ToolConsole::Progress(
			    std::cout,
			    "Cooking",
			    "texture",
			    requestIndex + 1u,
			    requests.size(),
			    TextureCookerGetRequestDisplayName(request),
			    {ToolConsole::Field("assetId", Formatting::FormatHexUInt64(request.assetId)),
			     ToolConsole::Field("group", GetTextureGroupName(request.policy.textureGroup)),
			     ToolConsole::Field("dimension", GetTextureDimensionName(request.policy.dimension)),
			     ToolConsole::PathField("source", request.sourcePath),
			     ToolConsole::PathField("output", request.outputPath)});

			if (!TryProcessRequest(request, cooker, cookedCount, errorMessage))
			{
				ToolConsole::Error(errorMessage);
				return TextureCookerConstants::ExitCookFailed;
			}

			TextureCookRequestTiming timing;
			timing.elapsedMilliseconds = TextureCookerElapsedMilliseconds(requestStartTime);
			timing.assetId = request.assetId;
			timing.sourcePath = request.sourcePath;
			timing.outputPath = request.outputPath;
			requestTimings.push_back(timing);
		}


		std::ranges::sort(
		    requestTimings,
		    [](const TextureCookRequestTiming& lhs, const TextureCookRequestTiming& rhs) noexcept
		    {
			    return lhs.elapsedMilliseconds > rhs.elapsedMilliseconds;
		    });
		const std::size_t topCount = (std::min<std::size_t>)(requestTimings.size(), 10u);
		for (std::size_t timingIndex = 0; timingIndex < topCount; ++timingIndex)
		{
			const TextureCookRequestTiming& timing = requestTimings[timingIndex];
		}

		const std::uint64_t elapsedMilliseconds = TextureCookerElapsedMilliseconds(batchStartTime);
		PrintSummary(requestFilePath, requests.size(), cookedCount, elapsedMilliseconds, requestTimings);
		if (!summaryPath.empty())
		{
			std::string summaryError;
			if (!WriteSummary(
			        summaryPath,
			        requestFilePath,
			        requests.size(),
			        cookedCount,
			        elapsedMilliseconds,
			        requestTimings,
			        summaryError))
			{
				ToolConsole::Message(
				    std::cerr,
				    ToolConsoleSeverity::Error,
				    "Failed to write timing summary",
				    {ToolConsole::PathField("summary", summaryPath), ToolConsole::QuotedField("reason", summaryError)});
				return TextureCookerConstants::ExitCookFailed;
			}

			ToolConsole::Message(
			    std::cout,
			    ToolConsoleSeverity::Info,
			    "Timing summary written",
			    {ToolConsole::PathField("summary", summaryPath)});
		}
		return TextureCookerConstants::ExitSuccess;
	}

	bool TextureCookRequestBatchProcessor::TryLoadRequests(
		const std::filesystem::path& requestFilePath,
		std::vector<TextureCookRequest>& outRequests,
		std::string& outErrorMessage)
	{
		return LoadTextureCookRequestList(requestFilePath, outRequests, outErrorMessage);
	}

	bool TextureCookRequestBatchProcessor::TryProcessRequest(
		const TextureCookRequest& request,
		TextureAssetCooker& cooker,
		std::size_t& outCookedCount,
		std::string& outErrorMessage) const
	{
		if (!cooker.Cook(request, outErrorMessage))
		{
			outErrorMessage = "failed to cook texture '" + request.sourcePath.string() + "' - " + outErrorMessage;
			return false;
		}
		++outCookedCount;
		outErrorMessage.clear();
		return true;
	}

	void TextureCookRequestBatchProcessor::PrintSummary(
		const std::filesystem::path& requestFilePath,
		std::size_t requestCount,
		std::size_t cookedCount,
		std::uint64_t elapsedMilliseconds,
		const std::vector<TextureCookRequestTiming>& requestTimings)
	{
		ToolConsole::Summary(
		    std::cout,
		    "TextureCooker summary",
		    {ToolConsole::Field("mode", "cook"),
		     ToolConsole::PathField("requestFile", requestFilePath),
		     ToolConsole::Field("elapsedMs", std::to_string(elapsedMilliseconds)),
		     ToolConsole::Field("textures", std::to_string(cookedCount) + "/" + std::to_string(requestCount))});

		const std::size_t topCount = (std::min<std::size_t>)(requestTimings.size(), 10u);
		if (topCount > 0u)
		{
			ToolConsole::ListHeader(std::cout, "Slowest texture cooks");
			for (std::size_t timingIndex = 0; timingIndex < topCount; ++timingIndex)
			{
				const TextureCookRequestTiming& timing = requestTimings[timingIndex];
				ToolConsole::ListItem(
				    std::cout,
				    timingIndex + 1u,
				    {ToolConsole::QuotedField("name", ToolConsole::PathDisplayName(timing.sourcePath)),
				     ToolConsole::Field("elapsedMs", std::to_string(timing.elapsedMilliseconds)),
				     ToolConsole::Field("assetId", Formatting::FormatHexUInt64(timing.assetId)),
				     ToolConsole::PathField("source", timing.sourcePath)});
			}
		}
	}

	bool TextureCookRequestBatchProcessor::WriteSummary(
		const std::filesystem::path& summaryPath,
		const std::filesystem::path& requestFilePath,
		std::size_t requestCount,
		std::size_t cookedCount,
		std::uint64_t elapsedMilliseconds,
		const std::vector<TextureCookRequestTiming>& requestTimings,
		std::string& outErrorMessage)
	{
		auto writeRequests = [](const std::vector<TextureCookRequestTiming>& timings, std::size_t count)
		{
			std::ostringstream requests;
			requests << "[\n";
			for (std::size_t timingIndex = 0; timingIndex < count; ++timingIndex)
			{
				const TextureCookRequestTiming& timing = timings[timingIndex];
				requests << "    {"
				         << "\"rank\": " << (timingIndex + 1u) << ", "
				         << "\"name\": " << Json::QuoteString(ToolConsole::PathDisplayName(timing.sourcePath)) << ", "
				         << "\"elapsedMs\": " << timing.elapsedMilliseconds << ", "
				         << "\"assetId\": " << Json::QuoteString(Formatting::FormatHexUInt64(timing.assetId)) << ", "
				         << "\"status\": " << Json::QuoteString("cooked") << ", "
				         << "\"source\": " << Json::QuoteString(timing.sourcePath.generic_string()) << ", "
				         << "\"output\": " << Json::QuoteString(timing.outputPath.generic_string()) << "}";
				if (timingIndex + 1u < count)
				{
					requests << ',';
				}
				requests << "\n";
			}
			requests << "  ]";
			return requests.str();
		};

		const std::size_t topCount = (std::min<std::size_t>)(requestTimings.size(), 10u);
		const std::string topRequests = writeRequests(requestTimings, topCount);
		const std::string allRequests = writeRequests(requestTimings, requestTimings.size());

		Json::ObjectWriter writer;
		writer.WriteString("schema", "texture-cooker-summary-v1");
		writer.WriteString("tool", TextureCookerConstants::ToolName);
		writer.WriteString("mode", "cook-request-file");
		writer.WriteString("requestFile", requestFilePath.generic_string());
		writer.WriteUInt64("elapsedMs", elapsedMilliseconds);
		writer.WriteUInt64("requestCount", static_cast<std::uint64_t>(requestCount));
		writer.WriteUInt64("cookedCount", static_cast<std::uint64_t>(cookedCount));
		writer.WriteRaw("topRequests", topRequests);
		writer.WriteRaw("allRequests", allRequests);

		return Files::TryWriteAllTextAtomic(summaryPath, writer.Finish(), outErrorMessage);
	}

