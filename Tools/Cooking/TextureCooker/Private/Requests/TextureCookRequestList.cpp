#include "TextureCookRequestList.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Strings/StringUtils.h"
#include "TextureCookRequestCodec.h"

#include <algorithm>
#include <fstream>
#include <ranges>
#include <sstream>

class TextureCookRequestOrdering final
{
public:
	static void SortForSerialization(std::vector<TextureCookRequest>& requests)
	{
		std::ranges::sort(
		    requests,
		    [](const TextureCookRequest& lhs, const TextureCookRequest& rhs)
		    {
			    return lhs.assetId != rhs.assetId ? lhs.assetId < rhs.assetId
			                                      : lhs.outputPath.generic_string() < rhs.outputPath.generic_string();
		    });
	}

	static void SortForConsumption(std::vector<TextureCookRequest>& requests)
	{
		std::ranges::sort(
		    requests,
		    [](const TextureCookRequest& lhs, const TextureCookRequest& rhs) noexcept { return lhs.assetId < rhs.assetId; });
	}
};

void WriteTextureCookRequestList(const std::filesystem::path& outputPath, const std::vector<TextureCookRequest>& requests)
{
	if (outputPath.empty())
	{
		throw Diagnostics::Error("Texture cook request output path is empty.");
	}

	std::vector<TextureCookRequest> sortedRequests = requests;
	TextureCookRequestOrdering::SortForSerialization(sortedRequests);
	std::ostringstream output;
	output << TextureCookRequestCodec::GetHeader() << '\n';
	for (const TextureCookRequest& request : sortedRequests)
	{
		ValidateTextureCookRequest(request);
		output << TextureCookRequestCodec::FormatLine(request) << '\n';
	}

	std::string fileError;
	if (!Files::TryWriteAllText(outputPath, output.str(), fileError))
	{
		throw Diagnostics::Error(std::move(fileError));
	}
}

std::vector<TextureCookRequest> LoadTextureCookRequestList(const std::filesystem::path& inputPath)
{
	std::ifstream input(inputPath);
	if (!input.is_open())
	{
		throw Diagnostics::Error("Failed to open texture cook request file '" + inputPath.string() + "'.");
	}

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
				throw Diagnostics::Error("Texture cook request file '" + inputPath.string() + "' has an invalid header.");
			}
			foundHeader = true;
			continue;
		}

		try
		{
			requestSet.Add(TextureCookRequestCodec::ParseLine(trimmedLine));
		}
		catch (const Diagnostics::Error& error)
		{
			throw Diagnostics::Error(
			    std::string(error.what()) + " File: '" + inputPath.string() + "', line " + std::to_string(lineNumber) + ".");
		}
	}

	if (!foundHeader)
	{
		throw Diagnostics::Error("Texture cook request file '" + inputPath.string() + "' is empty.");
	}
	std::vector<TextureCookRequest> requests = requestSet.ReleaseRequests();
	TextureCookRequestOrdering::SortForConsumption(requests);
	return requests;
}
