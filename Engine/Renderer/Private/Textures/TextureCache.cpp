#include "../PCH.h"

#include "Textures/TextureCache.h"

#include "Assets/Cooked/CookedTextureReference.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Core/Public/FileSystemUtils.h"
#include "RHI/Public/Commands/RenderCommandList.h"
#include "RHI/Public/Commands/RhiCommandSubmissionService.h"
#include "RHI/Public/Descriptors/RhiDescriptorService.h"
#include "RHI/Public/Resources/RhiResourceService.h"
#include "RHI/Public/Resources/RhiUploadService.h"
#include "Tasks/Public/TaskExecutor.h"
#include "Tasks/Public/TaskScope.h"
#include "Textures/CookedTextureLoader.h"
#include "Textures/TextureDiagnosticsSnapshotBuilder.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <format>
#include <functional>
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
		QueueTextureRetirement(std::move(texture->second), nextBindingRevision);
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

void TextureCache::LoadDefaultTextures(RenderCommandList& commandList, std::vector<RhiResourceHandle>& uploadedResources)
{
	if (m_defaultsLoaded)
	{
		return;
	}

	for (std::uint8_t index = 0; index < static_cast<std::uint8_t>(DefaultTexture::Count); ++index)
	{
		const DefaultTexture type = static_cast<DefaultTexture>(index);
		const std::filesystem::path& texturePath = DefaultTextures::GetPath(type);
		const std::optional<ResolvedTexturePath> source = ResolveTexturePath(texturePath);
		if (!source)
		{
			Diagnostics::Fatal(
			    g_textureCacheLogger,
			    __FILE__,
			    __LINE__,
			    std::format("Default texture '{}' could not be resolved.", texturePath.string()));
		}

		if (m_defaultTextures.contains(source->CacheKey))
		{
			continue;
		}

		LoadedTextureData decodedTexture;
		try
		{
			decodedTexture = CookedTextureLoader::Decode(CookedTextureLoader::Read(source->Path));
		}
		catch (const Diagnostics::Error& error)
		{
			Diagnostics::Fatal(
			    g_textureCacheLogger,
			    __FILE__,
			    __LINE__,
			    std::format("Default texture '{}' failed to load: {}", source->Path.string(), error.what()));
		}

		RendererTexture texture = m_textureFactory.Create(source->Path, decodedTexture, commandList);
		uploadedResources.push_back(m_resourceService.GetResourceHandle(texture.Resource));
		m_defaultTextures.emplace(source->CacheKey, std::move(texture));
	}

	const std::optional<ResolvedTexturePath> checker = ResolveTexturePath(DefaultTextures::GetPath(DefaultTexture::Checkerboard));
	const std::optional<ResolvedTexturePath> sky = ResolveTexturePath(DefaultTextures::GetPath(DefaultTexture::Sky));
	if (!checker || !sky)
	{
		Diagnostics::Fatal(g_textureCacheLogger, __FILE__, __LINE__, "Checker or sky texture slot could not be resolved.");
	}
	m_textureSlotKeys[static_cast<std::size_t>(TextureId::Checker)] = checker->CacheKey;
	m_textureSlotKeys[static_cast<std::size_t>(TextureId::DefaultSky)] = sky->CacheKey;
	m_defaultsLoaded = true;
	++m_bindingRevision;
}

void TextureCache::RequestTexture(const ResolvedTexturePath& source, std::uint32_t generation)
{
	if (m_defaultTextures.contains(source.CacheKey))
	{
		return;
	}
	const auto active = m_pathTextures.find(source.CacheKey);
	if (active != m_pathTextures.end() && active->second.Generation.Generation == generation)
	{
		return;
	}
	if (FindRequest(source.CacheKey, generation) != nullptr)
	{
		return;
	}

	const std::optional<AssetGenerationHandle> handle = m_residency.BeginGeneration(MakeAssetKey(source.CacheKey), generation);
	if (!handle)
	{
		Diagnostics::Fatal(g_textureCacheLogger, __FILE__, __LINE__, "Texture generation could not enter residency.");
	}
	if (m_taskScope == nullptr)
	{
		Diagnostics::Fatal(g_textureCacheLogger, __FILE__, __LINE__, "Texture cache has no task scope.");
	}

	TextureRequest request;
	request.Source = source;
	request.Generation = *handle;
	request.Payload = std::make_shared<CookedTextureLoadTask::Payload>();
	m_requests.push_back(std::move(request));
	LaunchPendingRequests();
}

void TextureCache::SynchronizeSceneTextures(const RenderTextureTable& textures)
{
	if (textures.Generation == 0)
	{
		Diagnostics::Fatal(g_textureCacheLogger, __FILE__, __LINE__, "Scene texture table has no generation identity.");
	}
	const std::uint32_t generation = textures.Generation;
	if (m_sceneTextureGeneration == generation)
	{
		return;
	}

	m_wantedPathTextureKeys.clear();
	for (const RenderTextureAsset& texture : textures.Assets)
	{
		const std::optional<ResolvedTexturePath> source = ResolveTexturePath(texture.Path);
		if (!source)
		{
			Diagnostics::Fatal(
			    g_textureCacheLogger,
			    __FILE__,
			    __LINE__,
			    std::format("Cooked scene texture '{}' could not be resolved.", texture.Path.string()));
		}

		m_wantedPathTextureKeys.insert(source->CacheKey);
		RequestTexture(*source, generation);
	}

	for (TextureRequest& request : m_requests)
	{
		request.Wanted = m_wantedPathTextureKeys.contains(request.Source.CacheKey);
		if (!request.Wanted)
		{
			(void) m_residency.Cancel(request.Generation);
		}
	}

	bool retiredTexture = false;
	const std::uint64_t nextBindingRevision = m_bindingRevision + 1u;
	for (auto texture = m_pathTextures.begin(); texture != m_pathTextures.end();)
	{
		if (m_wantedPathTextureKeys.contains(texture->first))
		{
			++texture;
			continue;
		}

		QueueTextureRetirement(std::move(texture->second), nextBindingRevision);
		texture = m_pathTextures.erase(texture);
		retiredTexture = true;
	}

	if (retiredTexture || m_sceneTextureGeneration != generation)
	{
		m_bindingRevision = nextBindingRevision;
	}
	m_sceneTextureGeneration = generation;
}

void TextureCache::LaunchPendingRequests()
{
	const std::size_t activeLoadCount = static_cast<std::size_t>(std::count_if(
	    m_requests.begin(),
	    m_requests.end(),
	    [](const TextureRequest& request) noexcept { return request.LoadStarted && request.Execution.IsValid(); }));
	std::size_t availableSlots = activeLoadCount < kMaximumConcurrentLoads ? kMaximumConcurrentLoads - activeLoadCount : 0;
	for (TextureRequest& request : m_requests)
	{
		if (availableSlots == 0)
		{
			return;
		}
		if (request.LoadStarted || !request.Wanted || request.Payload == nullptr)
		{
			continue;
		}

		LaunchRequest(request);
		--availableSlots;
	}
}

void TextureCache::LaunchRequest(TextureRequest& request)
{
	request.Execution = CookedTextureLoadTask::Launch(m_taskExecutor, *m_taskScope, request.Source.Path, request.Payload);
	request.LoadStarted = true;
}

void TextureCache::ConsumeCompletedRequests() noexcept
{
	for (TextureRequest& request : m_requests)
	{
		if (!request.Execution.IsValid() || !request.Execution.IsSettled())
		{
			continue;
		}

		const TaskExecutionStatus executionStatus = request.Execution.GetStatus();
		const TaskResult executionResult = request.Execution.GetResult();
		request.Execution = {};
		if (!request.Wanted)
		{
			(void) m_residency.Cancel(request.Generation);
			request.Payload.reset();
			continue;
		}
		if (executionStatus != TaskExecutionStatus::Succeeded)
		{
			Diagnostics::Fatal(
			    g_textureCacheLogger,
			    __FILE__,
			    __LINE__,
			    std::format(
			        "Texture generation {} for '{}' failed: {}",
			        request.Generation.Generation,
			        request.Source.Path.string(),
			        executionResult.GetMessage()));
		}
		if (request.Payload == nullptr)
		{
			Diagnostics::Fatal(g_textureCacheLogger, __FILE__, __LINE__, "Texture task graph produced no decoded payload.");
		}

		if (!m_residency.BeginDecoding(request.Generation))
		{
			Diagnostics::Fatal(g_textureCacheLogger, __FILE__, __LINE__, "Texture generation could not enter decoding.");
		}
		const std::uint64_t decodedBytes = RendererTextureFactory::CalculatePayloadBytes(request.Payload->Texture.Upload);
		if (!m_residency.PublishReadyForUpload(request.Generation, decodedBytes, decodedBytes))
		{
			Diagnostics::Fatal(g_textureCacheLogger, __FILE__, __LINE__, "Texture generation exceeded residency capacity.");
		}

		request.Decoded.emplace(std::move(request.Payload->Texture));
		request.Payload.reset();
	}

	m_requests.erase(
	    std::remove_if(
	        m_requests.begin(),
	        m_requests.end(),
	        [this](const TextureRequest& request) noexcept
	        {
		        const AssetResidencyState state = m_residency.GetState(request.Generation);
		        return !request.Execution.IsValid() && !request.Decoded && !request.Uploaded && state == AssetResidencyState::Retired;
	        }),
	    m_requests.end());
}

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
			ActiveTexture staleTexture{.Generation = request.Generation, .Texture = std::move(*request.Uploaded)};
			ReleaseActiveTexture(staleTexture);
			request.Uploaded.reset();
			continue;
		}
		if (active != m_pathTextures.end())
		{
			const std::uint64_t nextBindingRevision = m_bindingRevision + 1u;
			QueueTextureRetirement(std::move(active->second), nextBindingRevision);
			active->second = ActiveTexture{.Generation = request.Generation, .Texture = std::move(*request.Uploaded)};
			m_bindingRevision = nextBindingRevision;
		}
		else
		{
			m_pathTextures.emplace(
			    request.Source.CacheKey,
			    ActiveTexture{.Generation = request.Generation, .Texture = std::move(*request.Uploaded)});
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

void TextureCache::QueueTextureRetirement(ActiveTexture&& texture, std::uint64_t bindingRevision)
{
	m_retiredTextures.push_back(RetiredTexture{.BindingRevision = bindingRevision, .Active = std::move(texture)});
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
