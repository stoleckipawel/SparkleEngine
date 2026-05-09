#include "TextureCookRequestList.h"

#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Strings/StringUtils.h"

#include <algorithm>
#include <fstream>
#include <map>
#include <sstream>
#include <string_view>

	static constexpr std::string_view kTextureCookRequestHeaderV2 = "TextureCookRequests|2";
	static constexpr std::string_view kTextureCookRequestHeaderV3 = "TextureCookRequests|3";
	static constexpr std::string_view kTextureCookRequestHeaderV4 = "TextureCookRequests|4";

	enum class TextureCookRequestFileVersion : std::uint8_t
	{
		Version2 = 2,
		Version3 = 3,
		Version4 = 4,
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

	static bool TryParseTextureGroup(std::string_view value, TextureGroup& outTextureGroup) noexcept
	{
		if (Strings::EqualsIgnoreCase(value, "default") || Strings::EqualsIgnoreCase(value, "none"))
		{
			outTextureGroup = TextureGroup::Default;
			return true;
		}

		if (Strings::EqualsIgnoreCase(value, "diffuse") || Strings::EqualsIgnoreCase(value, "albedo") ||
		    Strings::EqualsIgnoreCase(value, "base-color") || Strings::EqualsIgnoreCase(value, "basecolor") ||
		    Strings::EqualsIgnoreCase(value, "color"))
		{
			outTextureGroup = TextureGroup::Diffuse;
			return true;
		}

		if (Strings::EqualsIgnoreCase(value, "roughness"))
		{
			outTextureGroup = TextureGroup::Roughness;
			return true;
		}

		if (Strings::EqualsIgnoreCase(value, "metallic") || Strings::EqualsIgnoreCase(value, "metalness"))
		{
			outTextureGroup = TextureGroup::Metallic;
			return true;
		}

		if (Strings::EqualsIgnoreCase(value, "ambient-occlusion") || Strings::EqualsIgnoreCase(value, "ambientocclusion") ||
		    Strings::EqualsIgnoreCase(value, "occlusion") || Strings::EqualsIgnoreCase(value, "ao") ||
		    Strings::EqualsIgnoreCase(value, "masks"))
		{
			outTextureGroup = TextureGroup::AmbientOcclusion;
			return true;
		}

		if (Strings::EqualsIgnoreCase(value, "normal-map") || Strings::EqualsIgnoreCase(value, "normalmap"))
		{
			outTextureGroup = TextureGroup::NormalMap;
			return true;
		}

		if (Strings::EqualsIgnoreCase(value, "hdr-color") || Strings::EqualsIgnoreCase(value, "hdrcolor"))
		{
			outTextureGroup = TextureGroup::HdrColor;
			return true;
		}

		if (Strings::EqualsIgnoreCase(value, "emissive") || Strings::EqualsIgnoreCase(value, "emission"))
		{
			outTextureGroup = TextureGroup::Emissive;
			return true;
		}

		if (Strings::EqualsIgnoreCase(value, "subsurface-color") || Strings::EqualsIgnoreCase(value, "subsurfacecolor"))
		{
			outTextureGroup = TextureGroup::SubsurfaceColor;
			return true;
		}

		if (Strings::EqualsIgnoreCase(value, "subsurface-strength") || Strings::EqualsIgnoreCase(value, "subsurfacestrength"))
		{
			outTextureGroup = TextureGroup::SubsurfaceStrength;
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

	static bool TryParseTextureChannelMask(std::string_view value, TextureChannelMask& outChannelMask) noexcept
	{
		if (Strings::EqualsIgnoreCase(value, "rgba") || Strings::EqualsIgnoreCase(value, "all") || Strings::EqualsIgnoreCase(value, "none"))
		{
			outChannelMask = TextureChannelMask::Rgba;
			return true;
		}

		if (Strings::EqualsIgnoreCase(value, "r") || Strings::EqualsIgnoreCase(value, "red"))
		{
			outChannelMask = TextureChannelMask::Red;
			return true;
		}

		if (Strings::EqualsIgnoreCase(value, "g") || Strings::EqualsIgnoreCase(value, "green"))
		{
			outChannelMask = TextureChannelMask::Green;
			return true;
		}

		if (Strings::EqualsIgnoreCase(value, "b") || Strings::EqualsIgnoreCase(value, "blue"))
		{
			outChannelMask = TextureChannelMask::Blue;
			return true;
		}

		if (Strings::EqualsIgnoreCase(value, "a") || Strings::EqualsIgnoreCase(value, "alpha"))
		{
			outChannelMask = TextureChannelMask::Alpha;
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
		const std::size_t expectedFieldCount = version == TextureCookRequestFileVersion::Version4   ? 10u
		                                       : version == TextureCookRequestFileVersion::Version3 ? 9u
		                                                                                            : 4u;
		if (fields.size() != expectedFieldCount)
		{
			outErrorMessage = "Texture cook request entry is malformed.";
			return false;
		}

		const std::size_t outputPathIndex = version == TextureCookRequestFileVersion::Version4   ? 8u
		                                    : version == TextureCookRequestFileVersion::Version3 ? 7u
		                                                                                         : 2u;
		const std::size_t sourcePathIndex = version == TextureCookRequestFileVersion::Version4   ? 9u
		                                    : version == TextureCookRequestFileVersion::Version3 ? 8u
		                                                                                         : 3u;

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

		if (version == TextureCookRequestFileVersion::Version3 || version == TextureCookRequestFileVersion::Version4)
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

			if (!TryParseTextureGroup(fields[5], outRequest.textureGroup))
			{
				outErrorMessage = "Texture cook request entry has an unknown texture group '" + std::string(fields[5]) + "'.";
				return false;
			}

			if (!TryParseTextureDimension(fields[6], outRequest.dimension))
			{
				outErrorMessage = "Texture cook request entry has an unknown texture dimension '" + std::string(fields[6]) + "'.";
				return false;
			}

			if (version == TextureCookRequestFileVersion::Version4 && !TryParseTextureChannelMask(fields[7], outRequest.channelMask))
			{
				outErrorMessage = "Texture cook request entry has an unknown channel mask '" + std::string(fields[7]) + "'.";
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

	static bool TryParseRequestFileVersion(std::string_view headerLine, TextureCookRequestFileVersion& outVersion) noexcept
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

		if (headerLine == kTextureCookRequestHeaderV4)
		{
			outVersion = TextureCookRequestFileVersion::Version4;
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

	const char* GetTextureGroupName(TextureGroup textureGroup) noexcept
	{
		switch (textureGroup)
		{
			case TextureGroup::Default:
				return "default";
			case TextureGroup::Diffuse:
				return "diffuse";
			case TextureGroup::NormalMap:
				return "normal-map";
			case TextureGroup::Roughness:
				return "roughness";
			case TextureGroup::Metallic:
				return "metallic";
			case TextureGroup::AmbientOcclusion:
				return "ambient-occlusion";
			case TextureGroup::Emissive:
				return "emissive";
			case TextureGroup::SubsurfaceColor:
				return "subsurface-color";
			case TextureGroup::SubsurfaceStrength:
				return "subsurface-strength";
			case TextureGroup::HdrColor:
				return "hdr-color";
		}

		return "default";
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

	const char* GetTextureChannelMaskName(TextureChannelMask channelMask) noexcept
	{
		switch (channelMask)
		{
			case TextureChannelMask::Rgba:
				return "rgba";
			case TextureChannelMask::Red:
				return "red";
			case TextureChannelMask::Green:
				return "green";
			case TextureChannelMask::Blue:
				return "blue";
			case TextureChannelMask::Alpha:
				return "alpha";
		}

		return "rgba";
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
		output << kTextureCookRequestHeaderV4 << '\n';
		for (const TextureCookRequest& request : sortedRequests)
		{
			if (!request.IsValid())
			{
				outErrorMessage = "Texture cook request list contains an invalid request entry.";
				return false;
			}

			output << Formatting::FormatHexUInt64(request.assetId) << '|' << GetTextureColorSpaceName(request.colorSpace) << '|'
			       << GetTextureMipPolicyName(request.mipPolicy) << '|' << GetTextureMipFilterName(request.mipFilter) << '|'
			       << GetTextureColorProcessingPolicyName(request.colorProcessingPolicy) << '|' << GetTextureGroupName(request.textureGroup)
			       << '|' << GetTextureDimensionName(request.dimension) << '|' << GetTextureChannelMaskName(request.channelMask) << '|'
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
				    existingRequest.textureGroup != request.textureGroup || existingRequest.dimension != request.dimension ||
				    existingRequest.channelMask != request.channelMask)
				{
					outErrorMessage = "Texture cook request file contains conflicting requests for asset id '" +
					                  Formatting::FormatHexUInt64(request.assetId) + "'.";
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