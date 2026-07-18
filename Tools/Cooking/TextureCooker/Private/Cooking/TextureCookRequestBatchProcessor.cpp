#include "PCH.h"

#include "Cooking/TextureCookRequestBatchProcessor.h"

#include "Constants/TextureCookerConstants.h"
#include "Cooking/TextureCookBatchExecutor.h"

#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "ToolConsole.h"

#include <iostream>

namespace
{
	constexpr std::size_t TextureCookMemoryBudget = 1024ull * 1024ull * 1024ull;

	std::string RequestDisplayName(const TextureCookRequest& request)
	{
		return ToolConsole::PathDisplayName(request.sourcePath);
	}

	void CleanupStagedOutputs(const std::vector<TextureCookBatchItemResult>& results)
	{
		for (const TextureCookBatchItemResult& result : results)
			if (!result.StagedOutputPath.empty())
				Files::CleanupTemporaryFile(result.StagedOutputPath);
	}
}

int TextureCookRequestBatchProcessor::CookRequestFile(const std::filesystem::path& requestFilePath) const
{
	std::string errorMessage;
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

	TextureCookBatchExecutionResult execution = TextureCookBatchExecutor::Execute(requests, TextureCookMemoryBudget);
	bool succeeded = execution.Succeeded;
	for (std::size_t index = 0; index < requests.size(); ++index)
	{
		const TextureCookBatchItemResult& result = execution.Items[index];
		if (!result.Succeeded)
		{
			succeeded = false;
			ToolConsole::Message(
			    std::cerr,
			    ToolConsoleSeverity::Error,
			    "Texture cook failed",
			    {ToolConsole::Field("assetId", Formatting::FormatHexUInt64(requests[index].assetId)),
			     ToolConsole::PathField("source", requests[index].sourcePath),
			     ToolConsole::QuotedField("reason", result.Diagnostic)});
		}
	}
	if (!succeeded)
	{
		CleanupStagedOutputs(execution.Items);
		return TextureCookerConstants::ExitCookFailed;
	}

	std::vector<Files::FilePublication> publication;
	publication.reserve(requests.size());
	for (std::size_t index = 0; index < requests.size(); ++index)
		publication.push_back({execution.Items[index].StagedOutputPath, requests[index].outputPath});
	if (!Files::TryPublishFileSet(publication, errorMessage))
	{
		CleanupStagedOutputs(execution.Items);
		ToolConsole::Error("Failed to publish texture cook generation: " + errorMessage);
		return TextureCookerConstants::ExitCookFailed;
	}

	for (std::size_t index = 0; index < requests.size(); ++index)
	{
		ToolConsole::Progress(
		    std::cout,
		    "Cooked",
		    "texture",
		    index + 1,
		    requests.size(),
		    RequestDisplayName(requests[index]),
		    {ToolConsole::Field("assetId", Formatting::FormatHexUInt64(requests[index].assetId)),
		     ToolConsole::PathField("output", requests[index].outputPath)});
	}

	ToolConsole::Info(
	    "Texture cook peak admitted decompressed bytes: " + std::to_string(execution.PeakAdmittedBytes) +
	    " (budget=" + std::to_string(TextureCookMemoryBudget) + ").");
	return TextureCookerConstants::ExitSuccess;
}

bool TextureCookRequestBatchProcessor::TryLoadRequests(
    const std::filesystem::path& requestFilePath,
    std::vector<TextureCookRequest>& outRequests,
    std::string& outErrorMessage)
{
	return LoadTextureCookRequestList(requestFilePath, outRequests, outErrorMessage);
}
