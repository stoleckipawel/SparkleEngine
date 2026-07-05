#include "PCH.h"

#include "Cooking/TextureCookRequestBatchProcessor.h"

#include "Constants/TextureCookerConstants.h"
#include "Cooking/TextureAssetCooker.h"

#include "Core/Public/Formatting/HexFormat.h"

#include "ToolConsole.h"

#include <iostream>
#include <objbase.h>

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

	int TextureCookRequestBatchProcessor::CookRequestFile(const std::filesystem::path& requestFilePath) const
	{
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
		for (std::size_t requestIndex = 0; requestIndex < requests.size(); ++requestIndex)
		{
			const TextureCookRequest& request = requests[requestIndex];
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
