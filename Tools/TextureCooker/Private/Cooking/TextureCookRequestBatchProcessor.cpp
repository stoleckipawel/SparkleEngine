#include "PCH.h"

#include "Cooking/TextureCookRequestBatchProcessor.h"

#include "Constants/TextureCookerConstants.h"
#include "Cooking/TextureAssetCooker.h"
#include "Cooking/TextureCookArtifactKeyBuilder.h"

#include "Core/Public/Formatting/HexFormat.h"

#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Diagnostics/ScopedLogEvent.h"
#include "Core/Public/Diagnostics/Trace.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <objbase.h>

struct TextureCookRequestTiming final
{
	std::uint64_t elapsedMilliseconds = 0;
	TextureAssetId assetId = InvalidTextureAssetId;
	bool cooked = false;
	bool skipped = false;
	std::filesystem::path sourcePath;
};

static std::uint64_t TextureCookerElapsedMilliseconds(std::chrono::steady_clock::time_point startTime) noexcept
{
	return static_cast<std::uint64_t>(
	    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count());
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

	int TextureCookRequestBatchProcessor::CookRequestFile(const std::filesystem::path& requestFilePath) const
	{
		static const auto textureCookerLogger = Logging::GetOrCreateLogger("Tools.TextureCooker");
		SPARKLE_CPU_SCOPE("Tools.TextureCooker.CookRequestFile");
		SPARKLE_LOG_SCOPE(textureCookerLogger, spdlog::level::info, "TextureCooker.CookRequestFile");
		SPDLOG_LOGGER_INFO(textureCookerLogger, "TextureCooker requestFile='{}' loading", requestFilePath.string());

		std::string errorMessage;
		ScopedComInitializer comInitializer;
		if (!comInitializer.TryInitialize(errorMessage))
		{
			std::cerr << TextureCookerConstants::ToolName << ": " << errorMessage << "\n";
			return TextureCookerConstants::ExitComInitializationFailed;
		}

		std::vector<TextureCookRequest> requests;
		if (!TryLoadRequests(requestFilePath, requests, errorMessage))
		{
			std::cerr << TextureCookerConstants::ToolName << ": failed to load request file - " << errorMessage << "\n";
			return TextureCookerConstants::ExitLoadRequestFileFailed;
		}

		TextureAssetCooker cooker;
		std::size_t cookedCount = 0;
		std::size_t skippedCount = 0;
		std::vector<TextureCookRequestTiming> requestTimings;
		requestTimings.reserve(requests.size());
		for (std::size_t requestIndex = 0; requestIndex < requests.size(); ++requestIndex)
		{
			const TextureCookRequest& request = requests[requestIndex];
			const std::string requestScopeName = "Tools.TextureCooker.Request." + Formatting::FormatHexUInt64(request.assetId);
			SPARKLE_CPU_SCOPE(requestScopeName);
			SPARKLE_LOG_SCOPE(textureCookerLogger, spdlog::level::info, requestScopeName);
			const auto requestStartTime = std::chrono::steady_clock::now();
			std::cout << TextureCookerConstants::ToolName << ": [" << (requestIndex + 1u) << "/" << requests.size()
			          << "] processing texture '" << Formatting::FormatHexUInt64(request.assetId) << "' source='"
			          << request.sourcePath.string() << "'\n";

			const std::size_t previousCookedCount = cookedCount;
			const std::size_t previousSkippedCount = skippedCount;
			if (!TryProcessRequest(request, cooker, cookedCount, skippedCount, errorMessage))
			{
				std::cerr << TextureCookerConstants::ToolName << ": " << errorMessage << "\n";
				return TextureCookerConstants::ExitCookFailed;
			}

			if (cookedCount != previousCookedCount)
			{
				std::cout << TextureCookerConstants::ToolName << ": [" << (requestIndex + 1u) << "/" << requests.size()
				          << "] cooked texture '" << Formatting::FormatHexUInt64(request.assetId) << "'\n";
			}
			else if (skippedCount != previousSkippedCount)
			{
				std::cout << TextureCookerConstants::ToolName << ": [" << (requestIndex + 1u) << "/" << requests.size()
				          << "] skipped current texture '" << Formatting::FormatHexUInt64(request.assetId) << "'\n";
			}

			TextureCookRequestTiming timing;
			timing.elapsedMilliseconds = TextureCookerElapsedMilliseconds(requestStartTime);
			timing.assetId = request.assetId;
			timing.cooked = cookedCount != previousCookedCount;
			timing.skipped = skippedCount != previousSkippedCount;
			timing.sourcePath = request.sourcePath;
			requestTimings.push_back(timing);
			SPDLOG_LOGGER_INFO(
			    textureCookerLogger,
			    "TextureCooker request assetId={} status={} elapsedMs={} source='{}'",
			    Formatting::FormatHexUInt64(request.assetId),
			    timing.cooked ? "cooked" : timing.skipped ? "skipped" : "unchanged",
			    timing.elapsedMilliseconds,
			    request.sourcePath.string());
		}

		std::cout << TextureCookerConstants::ToolName << ": processed " << requests.size() << " texture asset(s) from request file '"
		          << requestFilePath.string() << "'; cooked=" << cookedCount << ", skipped=" << skippedCount << "\n";
		SPDLOG_LOGGER_INFO(
		    textureCookerLogger,
		    "TextureCooker processed requestFile='{}' requests={} cooked={} skipped={}",
		    requestFilePath.string(),
		    requests.size(),
		    cookedCount,
		    skippedCount);
		for (const TextureCookRequest& request : requests)
		{
			PrintProcessedRequest(request);
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
			SPDLOG_LOGGER_INFO(
			    textureCookerLogger,
			    "TextureCooker topRequest rank={} assetId={} status={} elapsedMs={} source='{}'",
			    timingIndex + 1u,
			    Formatting::FormatHexUInt64(timing.assetId),
			    timing.cooked ? "cooked" : timing.skipped ? "skipped" : "unchanged",
			    timing.elapsedMilliseconds,
			    timing.sourcePath.string());
		}

		PrintSummary(requestFilePath, requests.size(), cookedCount, skippedCount);
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
		std::size_t& outSkippedCount,
		std::string& outErrorMessage) const
	{
		Cook::CookArtifactKey artifactKey;
		const std::string identityScopeName = "Tools.TextureCooker.Identity." + Formatting::FormatHexUInt64(request.assetId);
		{
			SPARKLE_CPU_SCOPE(identityScopeName);
			SPARKLE_LOG_SCOPE(Logging::GetOrCreateLogger("Tools.TextureCooker"), spdlog::level::info, identityScopeName);
			if (!TextureCookArtifactKeyBuilder::TryBuild(request, artifactKey, outErrorMessage))
			{
				outErrorMessage = "failed to build cook identity for texture '" + request.sourcePath.string() + "' - " + outErrorMessage;
				return false;
			}
		}

		const std::string cacheScopeName = "Tools.TextureCooker.CacheCheck." + Formatting::FormatHexUInt64(request.assetId);
		{
			SPARKLE_CPU_SCOPE(cacheScopeName);
			SPARKLE_LOG_SCOPE(Logging::GetOrCreateLogger("Tools.TextureCooker"), spdlog::level::info, cacheScopeName);
			if (Cook::CookArtifactCache::IsCurrent(artifactKey, outErrorMessage))
			{
				++outSkippedCount;
				return true;
			}
		}

		if (!outErrorMessage.empty())
		{
			outErrorMessage =
			    "failed to inspect cooked texture metadata for '" + request.sourcePath.string() + "' - " + outErrorMessage;
			return false;
		}

		const std::string cookScopeName = "Tools.TextureCooker.CookRequest." + Formatting::FormatHexUInt64(request.assetId);
		{
			SPARKLE_CPU_SCOPE(cookScopeName);
			SPARKLE_LOG_SCOPE(Logging::GetOrCreateLogger("Tools.TextureCooker"), spdlog::level::info, cookScopeName);
			if (!cooker.Cook(request, outErrorMessage))
			{
				outErrorMessage = "failed to cook texture '" + request.sourcePath.string() + "' - " + outErrorMessage;
				return false;
			}
		}

		const std::string publishScopeName = "Tools.TextureCooker.Publish." + Formatting::FormatHexUInt64(request.assetId);
		{
			SPARKLE_CPU_SCOPE(publishScopeName);
			SPARKLE_LOG_SCOPE(Logging::GetOrCreateLogger("Tools.TextureCooker"), spdlog::level::info, publishScopeName);
			if (!Cook::CookArtifactCache::Publish(artifactKey, outErrorMessage))
			{
				outErrorMessage = "failed to publish cook metadata for texture '" + request.sourcePath.string() + "' - " + outErrorMessage;
				return false;
			}
		}

		++outCookedCount;
		outErrorMessage.clear();
		return true;
	}

	void TextureCookRequestBatchProcessor::PrintSummary(
		const std::filesystem::path& requestFilePath,
		std::size_t requestCount,
		std::size_t cookedCount,
		std::size_t skippedCount)
	{
		std::cout << TextureCookerConstants::ToolName << " Summary:\n"
		          << "  mode=cook\n"
		          << "  requestFile='" << requestFilePath.string() << "'\n"
		          << "  requests=" << requestCount << "\n"
		          << "  cooked=" << cookedCount << "\n"
		          << "  skipped=" << skippedCount << "\n";
	}

	void TextureCookRequestBatchProcessor::PrintProcessedRequest(const TextureCookRequest& request)
	{
		std::cout << "  Texture '" << Formatting::FormatHexUInt64(request.assetId) << "' output='" << request.outputPath.string()
		          << "'\n";
	}