#include "PCH.h"

#include "Cli/InspectTextureCookRequestFileCommand.h"

#include "Constants/TextureCookerConstants.h"

#include "Core/Public/Formatting/HexFormat.h"

#include <iostream>
#include <vector>

namespace AssetAuthoring
{
	bool InspectTextureCookRequestFileCommand::MatchesName(std::string_view commandName) noexcept
	{
		return commandName == TextureCookerConstants::InspectRequestFileCommand ||
		       commandName == TextureCookerConstants::InspectRequestFileAlias;
	}

	int InspectTextureCookRequestFileCommand::Execute(const std::filesystem::path& requestFilePath) const
	{
		std::vector<TextureCookRequest> requests;
		std::string errorMessage;
		if (!LoadTextureCookRequestList(requestFilePath, requests, errorMessage))
		{
			std::cerr << TextureCookerConstants::ToolName << ": failed to inspect request file - " << errorMessage << "\n";
			return TextureCookerConstants::ExitInspectRequestFileFailed;
		}

		std::cout << TextureCookerConstants::ToolName << ": request file='" << requestFilePath.string() << "' contains "
		          << requests.size() << " texture request(s)\n";
		for (const TextureCookRequest& request : requests)
		{
			PrintRequest(request);
		}

		PrintSummary(requestFilePath, requests.size());
		return TextureCookerConstants::ExitSuccess;
	}

	void InspectTextureCookRequestFileCommand::PrintRequest(const TextureCookRequest& request)
	{
		std::cout << "  Texture '" << Formatting::FormatHexUInt64(request.assetId) << "' colorSpace='"
		          << GetTextureColorSpaceName(request.colorSpace) << "' mipPolicy='" << GetTextureMipPolicyName(request.mipPolicy)
		          << "' mipFilter='" << GetTextureMipFilterName(request.mipFilter) << "' colorProcessing='"
		          << GetTextureColorProcessingPolicyName(request.colorProcessingPolicy) << "' compressionFamily='"
		          << GetTextureCompressionFamilyPreferenceName(request.compressionFamilyPreference) << "' dimension='"
		          << GetTextureDimensionName(request.dimension) << "' output='" << request.outputPath.string() << "' source='"
		          << request.sourcePath.string() << "'\n";
	}

	void InspectTextureCookRequestFileCommand::PrintSummary(
		const std::filesystem::path& requestFilePath,
		std::size_t requestCount)
	{
		std::cout << TextureCookerConstants::ToolName << " Summary:\n"
		          << "  mode=inspect\n"
		          << "  requestFile='" << requestFilePath.string() << "'\n"
		          << "  requests=" << requestCount << "\n";
	}
}