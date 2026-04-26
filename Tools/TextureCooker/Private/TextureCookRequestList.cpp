#include "PCH.h"

#include "TextureCookRequestList.h"

#include <cctype>
#include <charconv>
#include <cstring>
#include <format>
#include <fstream>
#include <map>
#include <system_error>

namespace AssetAuthoring
{
	static constexpr std::string_view kTextureCookRequestHeader = "TextureCookRequests|2";

	static std::string TrimCopy(std::string_view value)
	{
		std::size_t first = 0;
		while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first])) != 0)
		{
			++first;
		}

		std::size_t last = value.size();
		while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1])) != 0)
		{
			--last;
		}

		return std::string(value.substr(first, last - first));
	}

	static std::filesystem::path NormalizeRequestPath(std::string_view pathText)
	{
		return std::filesystem::path(std::string(pathText)).lexically_normal();
	}

	static bool TryParseTextureColorSpace(std::string_view value, TextureColorSpace& outColorSpace) noexcept
	{
		if (_stricmp(std::string(value).c_str(), "linear") == 0)
		{
			outColorSpace = TextureColorSpace::Linear;
			return true;
		}

		if (_stricmp(std::string(value).c_str(), "srgb") == 0)
		{
			outColorSpace = TextureColorSpace::Srgb;
			return true;
		}

		return false;
	}

	static bool ParseAssetId(std::string_view value, TextureAssetId& outAssetId) noexcept
	{
		unsigned long long parsedAssetId = 0;
		const char* begin = value.data();
		const char* end = value.data() + value.size();
		const auto parseResult = std::from_chars(begin, end, parsedAssetId, 16);
		if (parseResult.ec != std::errc() || parseResult.ptr != end)
		{
			return false;
		}

		outAssetId = static_cast<TextureAssetId>(parsedAssetId);
		return true;
	}

	static bool ParseRequestLine(std::string_view line, TextureCookRequest& outRequest, std::string& outErrorMessage)
	{
		std::string_view fields[4] = {};
		std::size_t fieldIndex = 0;
		std::size_t start = 0;

		while (fieldIndex < 3)
		{
			const std::size_t separatorIndex = line.find('|', start);
			if (separatorIndex == std::string_view::npos)
			{
				outErrorMessage = "Texture cook request entry is malformed.";
				return false;
			}

			fields[fieldIndex++] = line.substr(start, separatorIndex - start);
			start = separatorIndex + 1;
		}

		fields[3] = line.substr(start);
		if (fields[2].empty())
		{
			outErrorMessage = "Texture cook request entry is missing an output path.";
			return false;
		}

		if (fields[3].empty())
		{
			outErrorMessage = "Texture cook request entry is missing a source path.";
			return false;
		}

		if (!ParseAssetId(fields[0], outRequest.assetId))
		{
			outErrorMessage = "Texture cook request entry has an invalid asset id '" + std::string(fields[0]) + "'.";
			return false;
		}

		if (!TryParseTextureColorSpace(fields[1], outRequest.colorSpace))
		{
			outErrorMessage = "Texture cook request entry has an unknown color space '" + std::string(fields[1]) + "'.";
			return false;
		}

		outRequest.outputPath = NormalizeRequestPath(fields[2]);
		outRequest.sourcePath = NormalizeRequestPath(fields[3]);
		if (!outRequest.IsValid())
		{
			outErrorMessage = "Texture cook request entry is invalid after parsing.";
			return false;
		}

		outErrorMessage.clear();
		return true;
	}

	const char* GetTextureColorSpaceName(TextureColorSpace colorSpace) noexcept
	{
		switch (colorSpace)
		{
			case TextureColorSpace::Linear:
				return "linear";
			case TextureColorSpace::Srgb:
				return "srgb";
		}

		return "linear";
	}

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

		std::error_code errorCode;
		if (!outputPath.parent_path().empty())
		{
			std::filesystem::create_directories(outputPath.parent_path(), errorCode);
			if (errorCode)
			{
				outErrorMessage = "Failed to create texture cook request directory '" + outputPath.parent_path().string() + "'.";
				return false;
			}
		}

		std::vector<TextureCookRequest> sortedRequests = requests;
		std::sort(
		    sortedRequests.begin(),
		    sortedRequests.end(),
		    [](const TextureCookRequest& lhs, const TextureCookRequest& rhs)
		    {
			    if (lhs.assetId != rhs.assetId)
			    {
				    return lhs.assetId < rhs.assetId;
			    }

			    return lhs.outputPath.generic_string() < rhs.outputPath.generic_string();
		    });

		std::ofstream output(outputPath, std::ios::trunc);
		if (!output.is_open())
		{
			outErrorMessage = "Failed to open texture cook request output '" + outputPath.string() + "'.";
			return false;
		}

		output << kTextureCookRequestHeader << '\n';
		for (const TextureCookRequest& request : sortedRequests)
		{
			if (!request.IsValid())
			{
				outErrorMessage = "Texture cook request list contains an invalid request entry.";
				return false;
			}

			output << std::format("{:016X}", request.assetId) << '|' << GetTextureColorSpaceName(request.colorSpace) << '|'
			       << request.outputPath.generic_string() << '|' << request.sourcePath.generic_string() << '\n';
		}

		if (!output.good())
		{
			outErrorMessage = "Failed to write texture cook request output '" + outputPath.string() + "'.";
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
		std::map<TextureAssetId, TextureCookRequest> requestsById;
		bool foundHeader = false;
		std::size_t lineNumber = 0;

		for (std::string line; std::getline(input, line);)
		{
			++lineNumber;
			const std::string trimmedLine = TrimCopy(line);
			if (trimmedLine.empty())
			{
				continue;
			}

			if (!foundHeader)
			{
				if (trimmedLine != kTextureCookRequestHeader)
				{
					outErrorMessage = "Texture cook request file '" + inputPath.string() + "' has an invalid header.";
					return false;
				}

				foundHeader = true;
				continue;
			}

			TextureCookRequest request;
			if (!ParseRequestLine(trimmedLine, request, outErrorMessage))
			{
				outErrorMessage += " File: '" + inputPath.string() + "', line " + std::to_string(lineNumber);
				return false;
			}

			if (auto it = requestsById.find(request.assetId); it != requestsById.end())
			{
				const TextureCookRequest& existingRequest = it->second;
				if (existingRequest.sourcePath != request.sourcePath || existingRequest.outputPath != request.outputPath ||
				    existingRequest.colorSpace != request.colorSpace)
				{
					outErrorMessage =
					    "Texture cook request file contains conflicting requests for asset id '" + std::format("{:016X}", request.assetId) +
					    "'.";
					return false;
				}

				continue;
			}

			requestsById.emplace(request.assetId, std::move(request));
		}

		if (!foundHeader)
		{
			outErrorMessage = "Texture cook request file '" + inputPath.string() + "' is empty.";
			return false;
		}

		outRequests.reserve(requestsById.size());
		for (auto& [_, request] : requestsById)
		{
			outRequests.push_back(std::move(request));
		}

		outErrorMessage.clear();
		return true;
	}
}