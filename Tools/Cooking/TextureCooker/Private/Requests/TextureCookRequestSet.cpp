#include "TextureCookRequestList.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Formatting/HexFormat.h"

#include <utility>

void ValidateTextureCookRequest(const TextureCookRequest& request)
{
	if (request.assetId == InvalidTextureAssetId)
	{
		throw Diagnostics::Error("Texture cook request has an invalid asset id.");
	}
	if (request.sourcePath.empty())
	{
		throw Diagnostics::Error("Texture cook request has no source path.");
	}
	if (request.outputPath.empty())
	{
		throw Diagnostics::Error("Texture cook request has no output path.");
	}
}

void TextureCookRequestSet::Clear() noexcept
{
	m_requestIndices.clear();
	m_requests.clear();
}

void TextureCookRequestSet::Add(const TextureCookRequest& request)
{
	ValidateTextureCookRequest(request);
	const auto [position, inserted] = m_requestIndices.try_emplace(request.assetId, m_requests.size());
	if (inserted)
	{
		try
		{
			m_requests.push_back(request);
		}
		catch (...)
		{
			m_requestIndices.erase(position);
			throw;
		}
		return;
	}
	if (m_requests[position->second] != request)
	{
		throw Diagnostics::Error("Texture cook request conflict for asset id '" + Formatting::FormatHexUInt64(request.assetId) + "'.");
	}
}

std::vector<TextureCookRequest> TextureCookRequestSet::ReleaseRequests() noexcept
{
	m_requestIndices.clear();
	return std::exchange(m_requests, {});
}
