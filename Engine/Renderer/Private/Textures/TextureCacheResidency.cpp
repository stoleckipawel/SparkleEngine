#include "PCH.h"
#include "Textures/TextureCache.h"

#include "Core/Public/Diagnostics/Verify.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Paths/PathUtils.h"
#include "RHI/Public/Commands/RenderCommandList.h"
#include "RHI/Public/Commands/RhiCommandSubmissionService.h"
#include "RHI/Public/Resources/RhiResourceService.h"

#include <algorithm>
#include <functional>
#include <utility>

static const auto g_textureCacheLogger = Logging::GetOrCreateLogger("Renderer.TextureCache");

void TextureCache::UploadReadyTextures(RenderCommandList& commandList, std::vector<RhiResourceHandle>& uploadedResources)
{
	for (TextureRequest& request : m_requests)
	{
		if (!request.Wanted || !request.Decoded || request.Uploaded
		    || m_residency.GetState(request.Generation) != AssetResidencyState::ReadyForUpload
		    || !m_residency.BeginUpload(request.Generation))
		{
			continue;
		}

		request.Uploaded = m_textureFactory.Create(request.Source.Path, *request.Decoded, commandList);

		request.Decoded.reset();
		request.UploadedResource = m_resourceService.GetResourceHandle(request.Uploaded->Resource);
		uploadedResources.push_back(request.UploadedResource);
	}
}

TextureCache::TextureRequest* TextureCache::FindRequest(const TextureKey& cacheKey, std::uint32_t generation) noexcept
{
	return const_cast<TextureRequest*>(std::as_const(*this).FindRequest(cacheKey, generation));
}

const TextureCache::TextureRequest* TextureCache::FindRequest(const TextureKey& cacheKey, std::uint32_t generation) const noexcept
{
	const auto request = std::find_if(
	    m_requests.begin(),
	    m_requests.end(),
	    [&cacheKey, generation](const TextureRequest& candidate) noexcept
	    { return candidate.Source.CacheKey == cacheKey && candidate.Generation.Generation == generation; });
	return request != m_requests.end() ? &*request : nullptr;
}

void TextureCache::ActivateResidentRequests() noexcept
{
	for (TextureRequest& request : m_requests)
	{
		if (!request.Uploaded)
		{
			continue;
		}

		const AssetResidencyState state = m_residency.GetState(request.Generation);
		if (!request.Wanted && state == AssetResidencyState::Retired)
		{
			m_textureFactory.Release(*request.Uploaded);
			request.Uploaded.reset();
			continue;
		}
		if (!request.Wanted || m_residency.GetState(request.Generation) != AssetResidencyState::Resident)
		{
			continue;
		}

		auto active = m_pathTextures.find(request.Source.CacheKey);
		if (active != m_pathTextures.end() && active->second.Generation.Generation > request.Generation.Generation)
		{
			ActiveTexture staleTexture{.Generation = request.Generation, .Texture = *request.Uploaded};
			ReleaseActiveTexture(staleTexture);
			request.Uploaded.reset();
			continue;
		}
		if (active != m_pathTextures.end())
		{
			const std::uint64_t nextBindingRevision = m_bindingRevision + 1u;
			QueueTextureRetirement(active->second, nextBindingRevision);
			active->second = ActiveTexture{.Generation = request.Generation, .Texture = *request.Uploaded};
			m_bindingRevision = nextBindingRevision;
		}
		else
		{
			m_pathTextures.emplace(request.Source.CacheKey, ActiveTexture{.Generation = request.Generation, .Texture = *request.Uploaded});
			++m_bindingRevision;
		}
		request.Uploaded.reset();
	}

	m_requests.erase(
	    std::remove_if(
	        m_requests.begin(),
	        m_requests.end(),
	        [](const TextureRequest& request) noexcept
	        { return request.LoadStarted && !request.Execution.IsValid() && !request.Decoded && !request.Uploaded; }),
	    m_requests.end());
}

void TextureCache::QueueTextureRetirement(ActiveTexture texture, std::uint64_t bindingRevision)
{
	m_retiredTextures.push_back(RetiredTexture{.BindingRevision = bindingRevision, .Active = texture});
}

void TextureCache::ReleaseActiveTexture(ActiveTexture& texture) noexcept
{
	const RhiSubmissionState lastUse = CaptureLastSubmittedState();
	if (!m_residency.BeginEviction(texture.Generation, lastUse))
	{
		Diagnostics::Fatal(g_textureCacheLogger, __FILE__, __LINE__, "Resident texture could not enter eviction.");
	}
	m_textureFactory.Release(texture.Texture);
}

RhiSubmissionState TextureCache::CaptureLastSubmittedState() const noexcept
{
	RhiSubmissionState state;
	for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
	{
		state.MarkUsed(m_submissions.GetLastSubmittedToken(static_cast<ERhiQueueType>(queueIndex)));
	}
	return state;
}

const RendererTexture* TextureCache::FindPathTexture(const std::filesystem::path& texturePath) const noexcept
{
	const std::optional<ResolvedTexturePath> resolved = ResolveTexturePath(texturePath);
	if (!resolved)
	{
		return nullptr;
	}
	const auto defaultTexture = m_defaultTextures.find(resolved->CacheKey);
	if (defaultTexture != m_defaultTextures.end())
	{
		return &defaultTexture->second;
	}
	const auto texture = m_pathTextures.find(resolved->CacheKey);
	return texture != m_pathTextures.end() && texture->second.Generation.Generation == m_sceneTextureGeneration ? &texture->second.Texture
	                                                                                                            : nullptr;
}

bool TextureCache::HasPendingRequest(const std::filesystem::path& texturePath) const noexcept
{
	const std::optional<ResolvedTexturePath> resolved = ResolveTexturePath(texturePath);
	return resolved.has_value()
	    && std::any_of(
	        m_requests.begin(),
	        m_requests.end(),
	        [this, &resolved](const TextureRequest& request) noexcept
	        {
		        return request.Wanted && request.Source.CacheKey == resolved->CacheKey
		            && request.Generation.Generation == m_sceneTextureGeneration;
	        });
}

std::optional<TextureCache::ResolvedTexturePath> TextureCache::ResolveTexturePath(const std::filesystem::path& texturePath) const noexcept
{
	const auto resolvedPath = Filesystem::ResolveAssetPathNormalized(texturePath, AssetType::Texture);
	if (!resolvedPath)
	{
		return std::nullopt;
	}
	TextureKey cacheKey = Paths::MakePathKey(*resolvedPath);
	if (cacheKey.empty())
	{
		return std::nullopt;
	}
	return ResolvedTexturePath{.Path = *resolvedPath, .CacheKey = std::move(cacheKey)};
}

std::uint64_t TextureCache::MakeAssetKey(const TextureKey& cacheKey)
{
	const std::uint64_t value = static_cast<std::uint64_t>(std::hash<TextureKey>{}(cacheKey));
	if (value == 0)
	{
		Diagnostics::Fatal(g_textureCacheLogger, __FILE__, __LINE__, "Texture path hashes to the null asset identity.");
	}
	return value;
}
