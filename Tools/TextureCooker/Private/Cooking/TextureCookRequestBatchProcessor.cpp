#include "PCH.h"

#include "Cooking/TextureCookRequestBatchProcessor.h"

#include "Constants/TextureCookerConstants.h"
#include "Cooking/TextureAssetCooker.h"
#include "Cooking/TextureCookArtifactKeyBuilder.h"

#include "Core/Public/Formatting/HexFormat.h"

#include <iostream>
#include <objbase.h>

namespace AssetAuthoring
{
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
		for (const TextureCookRequest& request : requests)
		{
			if (!TryProcessRequest(request, cooker, cookedCount, skippedCount, errorMessage))
			{
				std::cerr << TextureCookerConstants::ToolName << ": " << errorMessage << "\n";
				return TextureCookerConstants::ExitCookFailed;
			}
		}

		std::cout << TextureCookerConstants::ToolName << ": processed " << requests.size() << " texture asset(s) from request file '"
		          << requestFilePath.string() << "'; cooked=" << cookedCount << ", skipped=" << skippedCount << "\n";
		for (const TextureCookRequest& request : requests)
		{
			PrintProcessedRequest(request);
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
		if (!TextureCookArtifactKeyBuilder::TryBuild(request, artifactKey, outErrorMessage))
		{
			outErrorMessage = "failed to build cook identity for texture '" + request.sourcePath.string() + "' - " + outErrorMessage;
			return false;
		}

		if (Cook::CookArtifactCache::IsCurrent(artifactKey, outErrorMessage))
		{
			++outSkippedCount;
			return true;
		}

		if (!outErrorMessage.empty())
		{
			outErrorMessage =
			    "failed to inspect cooked texture metadata for '" + request.sourcePath.string() + "' - " + outErrorMessage;
			return false;
		}

		if (!cooker.Cook(request, outErrorMessage))
		{
			outErrorMessage = "failed to cook texture '" + request.sourcePath.string() + "' - " + outErrorMessage;
			return false;
		}

		if (!Cook::CookArtifactCache::Publish(artifactKey, outErrorMessage))
		{
			outErrorMessage = "failed to publish cook metadata for texture '" + request.sourcePath.string() + "' - " + outErrorMessage;
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
}