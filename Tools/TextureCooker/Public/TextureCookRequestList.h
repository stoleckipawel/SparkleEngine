#pragma once

#include "Core/Public/Assets/TextureGroup.h"
#include "Core/Public/Assets/TextureProperties.h"

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

typedef std::uint64_t TextureAssetId;

inline constexpr TextureAssetId InvalidTextureAssetId = 0;

enum class TextureMipPolicy : std::uint8_t
{
	Generate = 0,
	PreserveExisting = 1,
	NoMips = 2,
};

enum class TextureMipFilter : std::uint8_t
{
	Regular = 0,
	Kaiser = 1,
	NormalAware = 2,
	Angular = 3,
};

enum class TextureColorProcessingPolicy : std::uint8_t
{
	Linear = 0,
	SrgbLinearize = 1,
};

struct TextureCookPolicy final
{
	TextureColorSpace colorSpace = TextureColorSpace::Linear;
	TextureMipPolicy mipPolicy = TextureMipPolicy::Generate;
	TextureMipFilter mipFilter = TextureMipFilter::Regular;
	TextureColorProcessingPolicy colorProcessingPolicy = TextureColorProcessingPolicy::Linear;
	TextureGroup textureGroup = TextureGroup::Default;
	TextureDimension dimension = TextureDimension::Texture2D;
	TextureChannelMask channelMask = TextureChannelMask::Rgba;

	bool IsSrgb() const noexcept { return colorSpace == TextureColorSpace::Srgb; }
	bool IsCube() const noexcept { return dimension == TextureDimension::TextureCube; }
};

struct TextureCookRequest final
{
	TextureAssetId assetId = InvalidTextureAssetId;
	std::filesystem::path sourcePath;
	std::filesystem::path outputPath;
	TextureCookPolicy policy;

	bool IsValid() const noexcept { return assetId != InvalidTextureAssetId && !sourcePath.empty() && !outputPath.empty(); }
	bool IsSrgb() const noexcept { return policy.IsSrgb(); }
	bool IsCube() const noexcept { return policy.IsCube(); }
	explicit operator bool() const noexcept { return IsValid(); }
};

class TextureCookRequestSet final
{
public:
	void Clear() noexcept;
	bool Add(const TextureCookRequest& request, std::string& outErrorMessage);
	void MoveRequestsTo(std::vector<TextureCookRequest>& outRequests);
	const std::vector<TextureCookRequest>& Requests() const noexcept { return requests; }

private:
	std::map<TextureAssetId, TextureCookRequest> requestsById;
	std::vector<TextureCookRequest> requests;
};

bool TextureCookPoliciesMatch(const TextureCookPolicy& lhs, const TextureCookPolicy& rhs) noexcept;
bool TextureCookRequestsMatch(const TextureCookRequest& lhs, const TextureCookRequest& rhs) noexcept;

const char* GetTextureColorSpaceName(TextureColorSpace colorSpace) noexcept;
const char* GetTextureMipPolicyName(TextureMipPolicy mipPolicy) noexcept;
const char* GetTextureMipFilterName(TextureMipFilter mipFilter) noexcept;
const char* GetTextureColorProcessingPolicyName(TextureColorProcessingPolicy colorProcessingPolicy) noexcept;
const char* GetTextureGroupName(TextureGroup textureGroup) noexcept;
const char* GetTextureDimensionName(TextureDimension dimension) noexcept;
const char* GetTextureChannelMaskName(TextureChannelMask channelMask) noexcept;

bool WriteTextureCookRequestList(
    const std::filesystem::path& outputPath,
    const std::vector<TextureCookRequest>& requests,
    std::string& outErrorMessage);

bool LoadTextureCookRequestList(
    const std::filesystem::path& inputPath,
    std::vector<TextureCookRequest>& outRequests,
    std::string& outErrorMessage);