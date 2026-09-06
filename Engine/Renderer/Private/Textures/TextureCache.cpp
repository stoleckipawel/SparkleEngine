#include "../PCH.h"

#include "Textures/TextureCache.h"

#include "Assets/Cooked/CookedTextureReference.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "RHI/Public/Commands/RenderCommandList.h"
#include "RHI/Public/Commands/RhiQueue.h"
#include "RHI/Public/Commands/RhiCommandSubmissionService.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Descriptors/RhiDescriptorService.h"
#include "RHI/Public/Resources/RhiResourceService.h"
#include "RHI/Public/Resources/RhiUploadService.h"
#include "Tasks/Public/TaskExecutor.h"
#include "Tasks/Public/TaskScope.h"
#include "Textures/TextureDiagnosticsSnapshotBuilder.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <utility>

static const auto g_textureCacheLogger = Logging::GetOrCreateLogger("Renderer.TextureCache");

TextureCache::TextureCache(
    RhiResourceService& resourceService,
    RhiDescriptorService& descriptorService,
    RhiUploadService& uploadService,
    RhiCommandSubmissionService& submissions,
    TaskExecutor& taskExecutor,
    TaskScope& applicationScope) :
    m_resourceService(resourceService),
    m_descriptorService(descriptorService),
    m_textureFactory(resourceService, descriptorService, uploadService),
    m_submissions(submissions),
    m_taskExecutor(taskExecutor),
    m_taskScope(
        std::make_unique<TaskScope>(TaskScopeDesc{TaskScopeKind::AssetGeneration, "Renderer texture generations"}, &applicationScope))
{
}

TextureCache::~TextureCache() noexcept
{
	if (m_taskScope != nullptr)
	{
		m_taskScope->Cancel();
		(void) m_taskScope->JoinFor(std::chrono::milliseconds::max());
	}
	UnloadAll();
}

void TextureCache::UpdateSceneTextures(const RenderTextureTable& textures, RenderDeviceServices& deviceServices)
{
	RenderCommandList& graphicsCommandList = deviceServices.GetCurrentGraphicsCommandList();
	const bool useCopyQueue = HasPendingSceneTextureUploads()
	    && deviceServices.GetRenderHardwareInterface().GetCapabilities().Queues.SupportsIndependent(ERhiQueueType::Copy);

	RhiCommandRecordingLease uploadLease;
	RenderCommandList* uploadCommandList = &graphicsCommandList;
	if (useCopyQueue)
	{
		uploadLease = deviceServices.AcquireCommandRecordingLease(ERhiQueueType::Copy);
		uploadCommandList = &uploadLease.GetCommandList();
	}

	const std::vector<RhiResourceHandle> uploadedResources = LoadSceneTextures(textures, *uploadCommandList);
	if (!useCopyQueue)
	{
		return;
	}

	const RhiSubmissionToken uploadToken = deviceServices.SubmitCommandRecordingLease(std::move(uploadLease));
	RecordUploadSubmission(uploadToken);
	deviceServices.QueueWait(ERhiQueueType::Graphics, uploadToken);
	for (const RhiResourceHandle resource : uploadedResources)
	{
		graphicsCommandList.TransitionResource(resource, ResourceState::Common, ResourceState::ShaderResource);
	}
}

std::vector<RhiResourceHandle> TextureCache::LoadSceneTextures(const RenderTextureTable& textures, RenderCommandList& commandList)
{
	std::vector<RhiResourceHandle> uploadedResources;
	LoadDefaultTextures(commandList, uploadedResources);
	SynchronizeSceneTextures(textures);

	ConsumeCompletedRequests();
	LaunchPendingRequests();
	UploadReadyTextures(commandList, uploadedResources);
	return uploadedResources;
}

bool TextureCache::HasPendingSceneTextureUploads() const noexcept
{
	return std::any_of(
	    m_requests.begin(),
	    m_requests.end(),
	    [this](const TextureRequest& request) noexcept
	    {
		    return request.Wanted && request.Decoded.has_value()
		        && m_residency.GetState(request.Generation) == AssetResidencyState::ReadyForUpload;
	    });
}

void TextureCache::RecordUploadSubmission(RhiSubmissionToken token) noexcept
{
	if (!token.IsValid())
	{
		for (const TextureRequest& request : m_requests)
		{
			if (request.Uploaded && !request.UploadSubmitted)
			{
				Diagnostics::Fatal(g_textureCacheLogger, __FILE__, __LINE__, "GPU texture upload completed without a submission token.");
			}
		}
		return;
	}

	for (TextureRequest& request : m_requests)
	{
		if (request.Uploaded && !request.UploadSubmitted)
		{
			if (!m_residency.RecordUploadSubmission(request.Generation, token, request.Uploaded->EstimatedByteSize))
			{
				Diagnostics::Fatal(g_textureCacheLogger, __FILE__, __LINE__, "GPU texture upload submission could not enter residency.");
			}
			request.UploadSubmitted = true;
		}
	}
}

void TextureCache::PollResidency() noexcept
{
	m_residency.Poll(m_submissions);
	ActivateResidentRequests();
}

void TextureCache::CommitBindingRevision(std::uint64_t bindingRevision) noexcept
{
	for (RetiredTexture& texture : m_retiredTextures)
	{
		if (texture.BindingRevision <= bindingRevision)
		{
			ReleaseActiveTexture(texture.Active);
		}
	}

	m_retiredTextures.erase(
	    std::remove_if(
	        m_retiredTextures.begin(),
	        m_retiredTextures.end(),
	        [bindingRevision](const RetiredTexture& texture) noexcept { return texture.BindingRevision <= bindingRevision; }),
	    m_retiredTextures.end());
}

void TextureCache::UnloadSceneTextures() noexcept
{
	bool removedTexture = false;
	const std::uint64_t nextBindingRevision = m_bindingRevision + 1u;
	for (TextureRequest& request : m_requests)
	{
		request.Wanted = false;
		(void) m_residency.Cancel(request.Generation);
	}

	for (auto texture = m_pathTextures.begin(); texture != m_pathTextures.end();)
	{
		QueueTextureRetirement(texture->second, nextBindingRevision);
		texture = m_pathTextures.erase(texture);
		removedTexture = true;
	}

	if (removedTexture)
	{
		m_bindingRevision = nextBindingRevision;
	}
	m_wantedPathTextureKeys.clear();
	m_sceneTextureGeneration = 0;
}

void TextureCache::UnloadAll() noexcept
{
	const bool hadActiveTextures = !m_pathTextures.empty() || !m_defaultTextures.empty();
	for (TextureRequest& request : m_requests)
	{
		request.Wanted = false;
		(void) m_residency.Cancel(request.Generation);
		if (request.Uploaded)
		{
			m_textureFactory.Release(*request.Uploaded);
			request.Uploaded.reset();
		}
	}
	m_requests.clear();

	for (auto& [cacheKey, texture] : m_defaultTextures)
	{
		(void) cacheKey;
		m_textureFactory.Release(texture);
	}
	m_defaultTextures.clear();
	for (auto& [cacheKey, texture] : m_pathTextures)
	{
		(void) cacheKey;
		ReleaseActiveTexture(texture);
	}
	m_pathTextures.clear();
	for (RetiredTexture& texture : m_retiredTextures)
	{
		ReleaseActiveTexture(texture.Active);
	}
	m_retiredTextures.clear();
	m_wantedPathTextureKeys.clear();
	m_textureSlotKeys = {};
	m_sceneTextureGeneration = 0;
	m_defaultsLoaded = false;
	if (hadActiveTextures)
	{
		++m_bindingRevision;
	}
}

const RendererTexture* TextureCache::GetTexture(TextureId id) const noexcept
{
	const std::size_t index = static_cast<std::size_t>(id);
	if (index >= kTextureCount || m_textureSlotKeys[index].empty())
	{
		return nullptr;
	}
	const auto defaultTexture = m_defaultTextures.find(m_textureSlotKeys[index]);
	if (defaultTexture != m_defaultTextures.end())
	{
		return &defaultTexture->second;
	}
	const auto texture = m_pathTextures.find(m_textureSlotKeys[index]);
	return texture != m_pathTextures.end() ? &texture->second.Texture : nullptr;
}

const RendererTexture* TextureCache::ResolveDefaultSkyTexture() const noexcept
{
	return GetTexture(TextureId::DefaultSky);
}

const RendererTexture* TextureCache::GetSceneTexture(const std::filesystem::path& texturePath) const noexcept
{
	return FindPathTexture(texturePath);
}

const RendererTexture* TextureCache::ResolveTextureReferenceOrSemanticDefault(
    const Assets::CookedTextureReference* textureReference,
    DefaultTexture defaultType) const
{
	if (textureReference != nullptr && textureReference->IsValid())
	{
		if (const RendererTexture* texture = GetSceneTexture(textureReference->texturePath))
		{
			return texture;
		}
		if (!HasPendingRequest(textureReference->texturePath))
		{
			return nullptr;
		}
	}

	return FindPathTexture(DefaultTextures::GetPath(defaultType));
}

TextureDiagnosticsSnapshot TextureCache::CaptureDiagnosticsSnapshot(const PreviewTextureResolver& resolvePreviewTexture) const
{
	TextureDiagnosticsSnapshotBuilder builder(m_descriptorService, resolvePreviewTexture, m_defaultTextures.size() + m_pathTextures.size());
	for (const auto& [cacheKey, texture] : m_defaultTextures)
	{
		builder.Add(texture, TextureDiagnosticsKind::DefaultPath, std::filesystem::path(cacheKey).generic_string(), false);
	}
	for (const auto& [cacheKey, texture] : m_pathTextures)
	{
		builder.Add(texture.Texture, TextureDiagnosticsKind::Scene, std::filesystem::path(cacheKey).generic_string(), true);
	}
	return std::move(builder).Build();
}
