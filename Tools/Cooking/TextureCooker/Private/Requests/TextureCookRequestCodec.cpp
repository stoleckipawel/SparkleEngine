#include "TextureCookRequestCodec.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Strings/StringUtils.h"
#include "TextureCookPolicyCodec.h"

#include <sstream>
#include <vector>

class TextureCookRequestParsing final
{
  public:
	static constexpr std::string_view Header = "TextureCookRequests|1";
	static constexpr std::size_t FieldCount = 10;

	static TextureAssetId ParseAssetId(std::string_view value)
	{
		std::uint64_t parsedAssetId = 0;
		if (!Formatting::TryParseHexUInt64(value, parsedAssetId) || parsedAssetId == InvalidTextureAssetId)
		{
			throw Diagnostics::Error("Texture cook request entry has an invalid asset id '" + std::string(value) + "'.");
		}
		return static_cast<TextureAssetId>(parsedAssetId);
	}

	static std::filesystem::path NormalizePath(std::string_view value)
	{
		return std::filesystem::path(std::string(value)).lexically_normal();
	}
};

std::string_view TextureCookRequestCodec::GetHeader() noexcept
{
	return TextureCookRequestParsing::Header;
}

bool TextureCookRequestCodec::IsHeader(std::string_view line) noexcept
{
	return line == TextureCookRequestParsing::Header;
}

TextureCookRequest TextureCookRequestCodec::ParseLine(std::string_view line)
{
	const std::vector<std::string_view> fields = Strings::Split(line, '|');
	if (fields.size() != TextureCookRequestParsing::FieldCount)
	{
		throw Diagnostics::Error("Texture cook request entry is malformed.");
	}
	if (fields[8].empty())
	{
		throw Diagnostics::Error("Texture cook request entry is missing an output path.");
	}
	if (fields[9].empty())
	{
		throw Diagnostics::Error("Texture cook request entry is missing a source path.");
	}

	TextureCookRequest request;
	request.assetId = TextureCookRequestParsing::ParseAssetId(fields[0]);
	request.policy.colorSpace = TextureCookPolicyCodec::ParseColorSpace(fields[1]);
	request.policy.mipPolicy = TextureCookPolicyCodec::ParseMipPolicy(fields[2]);
	request.policy.mipFilter = TextureCookPolicyCodec::ParseMipFilter(fields[3]);
	request.policy.colorProcessingPolicy = TextureCookPolicyCodec::ParseColorProcessingPolicy(fields[4]);
	request.policy.textureGroup = TextureCookPolicyCodec::ParseTextureGroup(fields[5]);
	request.policy.dimension = TextureCookPolicyCodec::ParseDimension(fields[6]);
	request.policy.channelMask = TextureCookPolicyCodec::ParseChannelMask(fields[7]);
	request.outputPath = TextureCookRequestParsing::NormalizePath(fields[8]);
	request.sourcePath = TextureCookRequestParsing::NormalizePath(fields[9]);
	ValidateTextureCookRequest(request);
	return request;
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
