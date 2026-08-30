#include "PCH.h"

#include "Cli/InspectTextureCookRequestFileCommand.h"

#include "Constants/TextureCookerConstants.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Formatting/HexFormat.h"

#include <iostream>
#include <vector>

bool InspectTextureCookRequestFileCommand::MatchesName(std::string_view commandName) noexcept
{
	return commandName == TextureCookerConstants::InspectRequestFileCommand;
}

int InspectTextureCookRequestFileCommand::Execute(const std::filesystem::path& requestFilePath) const
{
	std::vector<TextureCookRequest> requests;
	try
	{
		requests = LoadTextureCookRequestList(requestFilePath);
	}
	catch (const Diagnostics::Error& error)
	{
		std::cerr << TextureCookerConstants::ToolName << ": failed to inspect request file - " << error.what() << "\n";
		return TextureCookerConstants::ExitInspectRequestFileFailed;
	}

	std::cout << TextureCookerConstants::ToolName << ": request file='" << requestFilePath.string() << "' contains " << requests.size()
	          << " texture request(s)\n";
	for (const TextureCookRequest& request : requests)
	{
		PrintRequest(request);
	}

	return TextureCookerConstants::ExitSuccess;
}

void InspectTextureCookRequestFileCommand::PrintRequest(const TextureCookRequest& request)
{
	std::cout << "  Texture '" << Formatting::FormatHexUInt64(request.assetId) << "' colorSpace='"
	          << GetTextureColorSpaceName(request.policy.colorSpace) << "' mipPolicy='" << GetTextureMipPolicyName(request.policy.mipPolicy)
	          << "' mipFilter='" << GetTextureMipFilterName(request.policy.mipFilter) << "' colorProcessing='"
	          << GetTextureColorProcessingPolicyName(request.policy.colorProcessingPolicy) << "' textureGroup='"
	          << GetTextureGroupName(request.policy.textureGroup) << "' dimension='" << GetTextureDimensionName(request.policy.dimension)
	          << "' channelMask='" << GetTextureChannelMaskName(request.policy.channelMask) << "' output='" << request.outputPath.string()
	          << "' source='" << request.sourcePath.string() << "'\n";
}
