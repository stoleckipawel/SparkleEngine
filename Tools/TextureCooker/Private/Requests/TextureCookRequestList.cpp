#include "TextureCookRequestList.h"

#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Strings/StringUtils.h"

#include <algorithm>
#include <fstream>
#include <map>
#include <sstream>
#include <string_view>

namespace AssetAuthoring
{
	static constexpr std::string_view kTextureCookRequestHeaderV2 = "TextureCookRequests|2";
	static constexpr std::string_view kTextureCookRequestHeaderV3 = "TextureCookRequests|3";

	enum class TextureCookRequestFileVersion : std::uint8_t
	{
		Version2 = 2,
		Version3 = 3,
	};

	static std::filesystem::path NormalizeRequestPath(std::string_view pathText)
	{
		return std::filesystem::path(std::string(pathText)).lexically_normal();
	}

	static bool TryParseTextureColorSpace(std::string_view value, TextureColorSpace& outColorSpace) noexcept
	{
		if (Strings::EqualsIgnoreCase(value, "linear"))
		{
			outColorSpace = TextureColorSpace::Linear;
			return true;
		}

		if (Strings::EqualsIgnoreCase(value, "srgb"))
		{
			outColorSpace = TextureColorSpace::Srgb;
			return true;
		}

		return false;
	}

	static bool TryParseTextureMipPolicy(std::string_view value, TextureMipPolicy& outMipPolicy) noexcept
	{
		if (Strings::EqualsIgnoreCase(value, "generate"))
		{
			outMipPolicy = TextureMipPolicy::Generate;
			return true;
		}

		if (Strings::EqualsIgnoreCase(value, "preserve-existing") || Strings::EqualsIgnoreCase(value, "preserveexisting"))
		{
			outMipPolicy = TextureMipPolicy::PreserveExisting;
			return true;
		}

		if (Strings::EqualsIgnoreCase(value, "no-mips") || Strings::EqualsIgnoreCase(value, "nomips"))
		{
			outMipPolicy = TextureMipPolicy::NoMips;
			return true;
		}

		return false;
	}

	static bool TryParseTextureMipFilter(std::string_view value, TextureMipFilter& outMipFilter) noexcept
	{
		if (Strings::EqualsIgnoreCase(value, "regular"))
		{
			outMipFilter = TextureMipFilter::Regular;
			return true;
		}

		if (Strings::EqualsIgnoreCase(value, "kaiser"))
		{
			outMipFilter = TextureMipFilter::Kaiser;
			return true;
		}

		if (Strings::EqualsIgnoreCase(value, "normal-aware") || Strings::EqualsIgnoreCase(value, "normalaware"))
		{
			outMipFilter = TextureMipFilter::NormalAware;
			return true;
		}

		if (Strings::EqualsIgnoreCase(value, "angular"))
		{
			outMipFilter = TextureMipFilter::Angular;
			return true;
		}

		return false;
	}

	static bool TryParseTextureColorProcessingPolicy(
		std::string_view value,
		TextureColorProcessingPolicy& outColorProcessingPolicy) noexcept
	{
		if (Strings::EqualsIgnoreCase(value, "linear"))
		{
			outColorProcessingPolicy = TextureColorProcessingPolicy::Linear;
			return true;
		}

		if (Strings::EqualsIgnoreCase(value, "srgb-linearize") || Strings::EqualsIgnoreCase(value, "srgblinearize"))
		{
			outColorProcessingPolicy = TextureColorProcessingPolicy::SrgbLinearize;
			return true;
		}

		return false;
	}

	static bool TryParseTextureCompressionFamilyPreference(
		std::string_view value,
		TextureCompressionFamilyPreference& outCompressionFamilyPreference) noexcept
	{
		if (Strings::EqualsIgnoreCase(value, "none"))
		{
			outCompressionFamilyPreference = TextureCompressionFamilyPreference::None;
			return true;
		}

		if (Strings::EqualsIgnoreCase(value, "color"))
		{
			outCompressionFamilyPreference = TextureCompressionFamilyPreference::Color;
			return true;
		}

		if (Strings::EqualsIgnoreCase(value, "masks"))
		{
			outCompressionFamilyPreference = TextureCompressionFamilyPreference::Masks;
			return true;
		}

		if (Strings::EqualsIgnoreCase(value, "normal-map") || Strings::EqualsIgnoreCase(value, "normalmap"))
		{
			outCompressionFamilyPreference = TextureCompressionFamilyPreference::NormalMap;
			return true;
		}

		if (Strings::EqualsIgnoreCase(value, "hdr-color") || Strings::EqualsIgnoreCase(value, "hdrcolor"))
		{
			outCompressionFamilyPreference = TextureCompressionFamilyPreference::HdrColor;
			return true;
		}

		if (Strings::EqualsIgnoreCase(value, "cube-color") || Strings::EqualsIgnoreCase(value, "cubecolor"))
		{
			outCompressionFamilyPreference = TextureCompressionFamilyPreference::CubeColor;
			return true;
		}

		return false;
	}

	static bool TryParseTextureDimension(std::string_view value, TextureDimension& outDimension) noexcept
	{
		if (Strings::EqualsIgnoreCase(value, "2d") || Strings::EqualsIgnoreCase(value, "texture2d"))
		{
			outDimension = TextureDimension::Texture2D;
			return true;
		}

		if (Strings::EqualsIgnoreCase(value, "cube") || Strings::EqualsIgnoreCase(value, "texturecube"))
		{
			outDimension = TextureDimension::TextureCube;
			return true;
		}

		return false;
	}

	static bool ParseAssetId(std::string_view value, TextureAssetId& outAssetId) noexcept
	{
		std::uint64_t parsedAssetId = 0;
		if (!Formatting::TryParseHexUInt64(value, parsedAssetId))
		{
			return false;
		}

		outAssetId = static_cast<TextureAssetId>(parsedAssetId);
		return true;
	}

	static bool ParseRequestLine(
		std::string_view line,
		TextureCookRequestFileVersion version,
		TextureCookRequest& outRequest,
		std::string& outErrorMessage)
	{
		const std::vector<std::string_view> fields = Strings::Split(line, '|');
		const std::size_t expectedFieldCount = version == TextureCookRequestFileVersion::Version3 ? 9u : 4u;
		if (fields.size() != expectedFieldCount)
		{
			outErrorMessage = "Texture cook request entry is malformed.";
			return false;
		}

		const std::size_t outputPathIndex = version == TextureCookRequestFileVersion::Version3 ? 7u : 2u;
		const std::size_t sourcePathIndex = version == TextureCookRequestFileVersion::Version3 ? 8u : 3u;

		if (fields[outputPathIndex].empty())
		{
			outErrorMessage = "Texture cook request entry is missing an output path.";
			return false;
		}

		if (fields[sourcePathIndex].empty())
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

		if (version == TextureCookRequestFileVersion::Version3)
		{
			if (!TryParseTextureMipPolicy(fields[2], outRequest.mipPolicy))
			{
				outErrorMessage = "Texture cook request entry has an unknown mip policy '" + std::string(fields[2]) + "'.";
				return false;
			}

			if (!TryParseTextureMipFilter(fields[3], outRequest.mipFilter))
			{
				outErrorMessage = "Texture cook request entry has an unknown mip filter '" + std::string(fields[3]) + "'.";
				return false;
			}

			if (!TryParseTextureColorProcessingPolicy(fields[4], outRequest.colorProcessingPolicy))
			{
				outErrorMessage = "Texture cook request entry has an unknown color processing policy '" + std::string(fields[4]) + "'.";
				return false;
			}

			if (!TryParseTextureCompressionFamilyPreference(fields[5], outRequest.compressionFamilyPreference))
			{
				outErrorMessage = "Texture cook request entry has an unknown compression family preference '" + std::string(fields[5]) + "'.";
				return false;
			}

			if (!TryParseTextureDimension(fields[6], outRequest.dimension))
			{
				outErrorMessage = "Texture cook request entry has an unknown texture dimension '" + std::string(fields[6]) + "'.";
				return false;
			}
		}

		outRequest.outputPath = NormalizeRequestPath(fields[outputPathIndex]);
		outRequest.sourcePath = NormalizeRequestPath(fields[sourcePathIndex]);
		if (!outRequest.IsValid())
		{
			outErrorMessage = "Texture cook request entry is invalid after parsing.";
			return false;
		}

		outErrorMessage.clear();
		return true;
	}

	static bool TryParseRequestFileVersion(
		std::string_view headerLine,
		TextureCookRequestFileVersion& outVersion) noexcept
	{
		if (headerLine == kTextureCookRequestHeaderV2)
		{
			outVersion = TextureCookRequestFileVersion::Version2;
			return true;
		}

		if (headerLine == kTextureCookRequestHeaderV3)
		{
			outVersion = TextureCookRequestFileVersion::Version3;
			return true;
		}

		return false;
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

	const char* GetTextureMipPolicyName(TextureMipPolicy mipPolicy) noexcept
	{
		switch (mipPolicy)
		{
			case TextureMipPolicy::Generate:
				return "generate";
			case TextureMipPolicy::PreserveExisting:
				return "preserve-existing";
			case TextureMipPolicy::NoMips:
				return "no-mips";
		}

		return "generate";
	}

	const char* GetTextureMipFilterName(TextureMipFilter mipFilter) noexcept
	{
		switch (mipFilter)
		{
			case TextureMipFilter::Regular:
				return "regular";
			case TextureMipFilter::Kaiser:
				return "kaiser";
			case TextureMipFilter::NormalAware:
				return "normal-aware";
			case TextureMipFilter::Angular:
				return "angular";
		}

		return "regular";
	}

	const char* GetTextureColorProcessingPolicyName(TextureColorProcessingPolicy colorProcessingPolicy) noexcept
	{
		switch (colorProcessingPolicy)
		{
			case TextureColorProcessingPolicy::Linear:
				return "linear";
			case TextureColorProcessingPolicy::SrgbLinearize:
				return "srgb-linearize";
		}

		return "linear";
	}

	const char* GetTextureCompressionFamilyPreferenceName(
		TextureCompressionFamilyPreference compressionFamilyPreference) noexcept
	{
		switch (compressionFamilyPreference)
		{
			case TextureCompressionFamilyPreference::None:
				return "none";
			case TextureCompressionFamilyPreference::Color:
				return "color";
			case TextureCompressionFamilyPreference::Masks:
				return "masks";
			case TextureCompressionFamilyPreference::NormalMap:
				return "normal-map";
			case TextureCompressionFamilyPreference::HdrColor:
				return "hdr-color";
			case TextureCompressionFamilyPreference::CubeColor:
				return "cube-color";
		}

		return "none";
	}

	const char* GetTextureDimensionName(TextureDimension dimension) noexcept
	{
		switch (dimension)
		{
			case TextureDimension::Texture2D:
				return "2d";
			case TextureDimension::TextureCube:
				return "cube";
		}

		return "2d";
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

		std::ostringstream output;
		output << kTextureCookRequestHeaderV3 << '\n';
		for (const TextureCookRequest& request : sortedRequests)
		{
			if (!request.IsValid())
			{
				outErrorMessage = "Texture cook request list contains an invalid request entry.";
				return false;
			}

			output << Formatting::FormatHexUInt64(request.assetId) << '|' << GetTextureColorSpaceName(request.colorSpace) << '|'
			       << GetTextureMipPolicyName(request.mipPolicy) << '|' << GetTextureMipFilterName(request.mipFilter) << '|'
			       << GetTextureColorProcessingPolicyName(request.colorProcessingPolicy) << '|'
			       << GetTextureCompressionFamilyPreferenceName(request.compressionFamilyPreference) << '|'
			       << GetTextureDimensionName(request.dimension) << '|'
			       << request.outputPath.generic_string() << '|' << request.sourcePath.generic_string() << '\n';
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
		std::map<TextureAssetId, TextureCookRequest> requestsById;
		bool foundHeader = false;
		TextureCookRequestFileVersion fileVersion = TextureCookRequestFileVersion::Version3;
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
				if (!TryParseRequestFileVersion(trimmedLine, fileVersion))
				{
					outErrorMessage = "Texture cook request file '" + inputPath.string() + "' has an invalid header.";
					return false;
				}

				foundHeader = true;
				continue;
			}

			TextureCookRequest request;
			if (!ParseRequestLine(trimmedLine, fileVersion, request, outErrorMessage))
			{
				outErrorMessage += " File: '" + inputPath.string() + "', line " + std::to_string(lineNumber);
				return false;
			}

			if (auto it = requestsById.find(request.assetId); it != requestsById.end())
			{
				const TextureCookRequest& existingRequest = it->second;
				if (existingRequest.sourcePath != request.sourcePath || existingRequest.outputPath != request.outputPath ||
				    existingRequest.colorSpace != request.colorSpace || existingRequest.mipPolicy != request.mipPolicy ||
				    existingRequest.mipFilter != request.mipFilter ||
				    existingRequest.colorProcessingPolicy != request.colorProcessingPolicy ||
				    existingRequest.compressionFamilyPreference != request.compressionFamilyPreference ||
				    existingRequest.dimension != request.dimension)
				{
					outErrorMessage =
					    "Texture cook request file contains conflicting requests for asset id '" + Formatting::FormatHexUInt64(request.assetId) +
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