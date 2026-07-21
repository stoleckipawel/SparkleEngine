#include "TextureCookRequestCodec.h"

#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Strings/StringUtils.h"
#include "TextureCookPolicyCodec.h"

#include <sstream>
#include <vector>

namespace
{
	constexpr std::string_view Header = "TextureCookRequests|1";
	constexpr std::size_t FieldCount = 10;

	bool ParseAssetId(std::string_view value, TextureAssetId& outAssetId) noexcept
	{
		std::uint64_t parsedAssetId = 0;
		if (!Formatting::TryParseHexUInt64(value, parsedAssetId))
		{
			return false;
		}
		outAssetId = static_cast<TextureAssetId>(parsedAssetId);
		return true;
	}

	std::filesystem::path NormalizePath(std::string_view value)
	{
		return std::filesystem::path(std::string(value)).lexically_normal();
	}
}

std::string_view TextureCookRequestCodec::GetHeader() noexcept
{
	return Header;
}

bool TextureCookRequestCodec::IsHeader(std::string_view line) noexcept
{
	return line == Header;
}

bool TextureCookRequestCodec::ParseLine(
    std::string_view line, TextureCookRequest& outRequest, std::string& outErrorMessage)
{
	const std::vector<std::string_view> fields = Strings::Split(line, '|');
	if (fields.size() != FieldCount)
	{
		outErrorMessage = "Texture cook request entry is malformed.";
		return false;
	}
	if (fields[8].empty())
	{
		outErrorMessage = "Texture cook request entry is missing an output path.";
		return false;
	}
	if (fields[9].empty())
	{
		outErrorMessage = "Texture cook request entry is missing a source path.";
		return false;
	}
	if (!ParseAssetId(fields[0], outRequest.assetId))
	{
		outErrorMessage = "Texture cook request entry has an invalid asset id '" + std::string(fields[0]) + "'.";
		return false;
	}
	if (!TextureCookPolicyCodec::TryParseColorSpace(fields[1], outRequest.policy.colorSpace))
	{
		outErrorMessage = "Texture cook request entry has an unknown color space '" + std::string(fields[1]) + "'.";
		return false;
	}
	if (!TextureCookPolicyCodec::TryParseMipPolicy(fields[2], outRequest.policy.mipPolicy))
	{
		outErrorMessage = "Texture cook request entry has an unknown mip policy '" + std::string(fields[2]) + "'.";
		return false;
	}
	if (!TextureCookPolicyCodec::TryParseMipFilter(fields[3], outRequest.policy.mipFilter))
	{
		outErrorMessage = "Texture cook request entry has an unknown mip filter '" + std::string(fields[3]) + "'.";
		return false;
	}
	if (!TextureCookPolicyCodec::TryParseColorProcessingPolicy(fields[4], outRequest.policy.colorProcessingPolicy))
	{
		outErrorMessage =
		    "Texture cook request entry has an unknown color processing policy '" + std::string(fields[4]) + "'.";
		return false;
	}
	if (!TextureCookPolicyCodec::TryParseTextureGroup(fields[5], outRequest.policy.textureGroup))
	{
		outErrorMessage = "Texture cook request entry has an unknown texture group '" + std::string(fields[5]) + "'.";
		return false;
	}
	if (!TextureCookPolicyCodec::TryParseDimension(fields[6], outRequest.policy.dimension))
	{
		outErrorMessage = "Texture cook request entry has an unknown texture dimension '" + std::string(fields[6]) + "'.";
		return false;
	}
	if (!TextureCookPolicyCodec::TryParseChannelMask(fields[7], outRequest.policy.channelMask))
	{
		outErrorMessage = "Texture cook request entry has an unknown channel mask '" + std::string(fields[7]) + "'.";
		return false;
	}

	outRequest.outputPath = NormalizePath(fields[8]);
	outRequest.sourcePath = NormalizePath(fields[9]);
	if (!outRequest.IsValid())
	{
		outErrorMessage = "Texture cook request entry is invalid after parsing.";
		return false;
	}
	outErrorMessage.clear();
	return true;
}

std::string TextureCookRequestCodec::FormatLine(const TextureCookRequest& request)
{
	std::ostringstream output;
	output << Formatting::FormatHexUInt64(request.assetId) << '|'
	       << GetTextureColorSpaceName(request.policy.colorSpace) << '|'
	       << GetTextureMipPolicyName(request.policy.mipPolicy) << '|'
	       << GetTextureMipFilterName(request.policy.mipFilter) << '|'
	       << GetTextureColorProcessingPolicyName(request.policy.colorProcessingPolicy) << '|'
	       << GetTextureGroupName(request.policy.textureGroup) << '|'
	       << GetTextureDimensionName(request.policy.dimension) << '|'
	       << GetTextureChannelMaskName(request.policy.channelMask) << '|'
	       << request.outputPath.generic_string() << '|' << request.sourcePath.generic_string();
	return output.str();
}
