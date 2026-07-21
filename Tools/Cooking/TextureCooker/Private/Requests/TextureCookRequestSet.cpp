#include "TextureCookRequestList.h"

#include "Core/Public/Formatting/HexFormat.h"

#include <utility>

bool TextureCookPoliciesMatch(const TextureCookPolicy& lhs, const TextureCookPolicy& rhs) noexcept
{
	return lhs.colorSpace == rhs.colorSpace && lhs.mipPolicy == rhs.mipPolicy && lhs.mipFilter == rhs.mipFilter &&
	       lhs.colorProcessingPolicy == rhs.colorProcessingPolicy && lhs.textureGroup == rhs.textureGroup &&
	       lhs.dimension == rhs.dimension && lhs.channelMask == rhs.channelMask;
}

bool TextureCookRequestsMatch(const TextureCookRequest& lhs, const TextureCookRequest& rhs) noexcept
{
	return lhs.assetId == rhs.assetId && lhs.sourcePath == rhs.sourcePath && lhs.outputPath == rhs.outputPath &&
	       TextureCookPoliciesMatch(lhs.policy, rhs.policy);
}

void TextureCookRequestSet::Clear() noexcept
{
	requestsById.clear();
	requests.clear();
}

bool TextureCookRequestSet::Add(const TextureCookRequest& request, std::string& outErrorMessage)
{
	const auto existingRequest = requestsById.find(request.assetId);
	if (existingRequest == requestsById.end())
	{
		requestsById.emplace(request.assetId, request);
		requests.push_back(request);
		outErrorMessage.clear();
		return true;
	}
	if (!TextureCookRequestsMatch(existingRequest->second, request))
	{
		outErrorMessage =
		    "Texture cook request conflict for asset id '" + Formatting::FormatHexUInt64(request.assetId) + "'.";
		return false;
	}
	outErrorMessage.clear();
	return true;
}

void TextureCookRequestSet::MoveRequestsTo(std::vector<TextureCookRequest>& outRequests)
{
	outRequests = std::move(requests);
	requestsById.clear();
	requests.clear();
}
