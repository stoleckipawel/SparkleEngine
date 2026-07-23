#include "TextureCookRequestList.h"

#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Strings/StringUtils.h"
#include "TextureCookRequestCodec.h"

#include <algorithm>
#include <fstream>
#include <ranges>
#include <sstream>

class TextureCookRequestListOperations final
{
  public:
	static void SortForSerialization(std::vector<TextureCookRequest>& requests)
	{
		std::ranges::sort(
		    requests,
		    [](const TextureCookRequest& lhs, const TextureCookRequest& rhs)
		    {
			    return lhs.assetId != rhs.assetId ?
			               lhs.assetId < rhs.assetId :
			               lhs.outputPath.generic_string() < rhs.outputPath.generic_string();
		    });
	}

	static void SortForConsumption(std::vector<TextureCookRequest>& requests)
	{
		std::ranges::sort(
		    requests,
		    [](const TextureCookRequest& lhs, const TextureCookRequest& rhs) noexcept
		    {
			    return lhs.assetId < rhs.assetId;
		    });
	}
};

bool WriteTextureCookRequestList(
    const std::filesystem::path& outputPath,
    const std::vector<TextureCookRequest>& requests,
    std::string& outErrorMessage)
{
	if (outputPath.empty())
	{
		outErrorMessage = "Texture cook request output path is empty.";
		return false;
	}

	std::vector<TextureCookRequest> sortedRequests = requests;
	TextureCookRequestListOperations::SortForSerialization(sortedRequests);
	std::ostringstream output;
	output << TextureCookRequestCodec::GetHeader() << '\n';
	for (const TextureCookRequest& request : sortedRequests)
	{
		if (!request.IsValid())
		{
			outErrorMessage = "Texture cook request list contains an invalid request entry.";
			return false;
		}
		output << TextureCookRequestCodec::FormatLine(request) << '\n';
	}

	if (!Files::TryWriteAllText(outputPath, output.str(), outErrorMessage))
	{
		return false;
	}
	outErrorMessage.clear();
	return true;
}

bool LoadTextureCookRequestList(
    const std::filesystem::path& inputPath,
    std::vector<TextureCookRequest>& outRequests,
    std::string& outErrorMessage)
{
	std::ifstream input(inputPath);
	if (!input.is_open())
	{
		outErrorMessage = "Failed to open texture cook request file '" + inputPath.string() + "'.";
		return false;
	}

	outRequests.clear();
	TextureCookRequestSet requestSet;
	bool foundHeader = false;
	std::size_t lineNumber = 0;
	for (std::string line; std::getline(input, line);)
	{
		++lineNumber;
		const std::string trimmedLine = Strings::TrimCopy(line);
		if (trimmedLine.empty())
		{
			continue;
		}
		if (!foundHeader)
		{
			if (!TextureCookRequestCodec::IsHeader(trimmedLine))
			{
				outErrorMessage = "Texture cook request file '" + inputPath.string() + "' has an invalid header.";
				return false;
			}
			foundHeader = true;
			continue;
		}

		TextureCookRequest request;
		if (!TextureCookRequestCodec::ParseLine(trimmedLine, request, outErrorMessage))
		{
			outErrorMessage += " File: '" + inputPath.string() + "', line " + std::to_string(lineNumber);
			return false;
		}
		if (!requestSet.Add(request, outErrorMessage))
		{
			outErrorMessage = "Texture cook request file contains conflicting requests for asset id '" +
			                  Formatting::FormatHexUInt64(request.assetId) + "'.";
			return false;
		}
	}

	if (!foundHeader)
	{
		outErrorMessage = "Texture cook request file '" + inputPath.string() + "' is empty.";
		return false;
	}
	requestSet.MoveRequestsTo(outRequests);
	TextureCookRequestListOperations::SortForConsumption(outRequests);
	outErrorMessage.clear();
	return true;
}
