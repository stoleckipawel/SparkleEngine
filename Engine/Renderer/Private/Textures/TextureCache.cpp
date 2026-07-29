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
#include "Tasks/Public/TaskGraph.h"
#include "Tasks/Public/TaskScope.h"

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
    m_uploadService(uploadService),
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
		    return request.Wanted && request.Decoded.has_value() &&
		           m_residency.GetState(request.Generation) == AssetResidencyState::ReadyForUpload;
	    });
}

void TextureCache::RecordUploadSubmission(RhiSubmissionToken token) noexcept
{
	if (!token.IsValid())
	{
		for (const TextureRequest& request : m_requests)
		{
			if (request.Uploaded && !request.UploadSubmitted)
				Diagnostics::Fatal(
				    g_textureCacheLogger,
				    __FILE__,
				    __LINE__,
				    "GPU texture upload completed without a submission token.");
		}
		return;
	}

	for (TextureRequest& request : m_requests)
	{
		if (request.Uploaded && !request.UploadSubmitted)
		{
			if (!m_residency.RecordUploadSubmission(request.Generation, token, request.Uploaded->EstimatedByteSize))
				Diagnostics::Fatal(
				    g_textureCacheLogger,
				    __FILE__,
				    __LINE__,
				    "GPU texture upload submission could not enter residency.");
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
	        [bindingRevision](const RetiredTexture& texture) noexcept
	        {
		        return texture.BindingRevision <= bindingRevision;
	        }),
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
			ReleaseTexture(*request.Uploaded);
			request.Uploaded.reset();
		}
	}
	m_requests.clear();

	for (auto& [cacheKey, texture] : m_defaultTextures)
	{
		(void) cacheKey;
		ReleaseTexture(texture);
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
		return GetSceneTexture(textureReference->texturePath);
	}

	return FindPathTexture(DefaultTextures::GetPath(defaultType));
}

TextureDiagnosticsSnapshot TextureCache::CaptureDiagnosticsSnapshot(const PreviewTextureResolver& resolvePreviewTexture) const
{
	TextureDiagnosticsSnapshot snapshot;
	snapshot.Rows.reserve(m_defaultTextures.size() + m_pathTextures.size());
	for (const auto& [cacheKey, texture] : m_defaultTextures)
	{
		if (!texture)
		{
			continue;
		}
		TextureDiagnosticsRow row = BuildDiagnosticsRow(
		    texture,
		    TextureDiagnosticsKind::DefaultPath,
		    std::filesystem::path(cacheKey).generic_string(),
		    resolvePreviewTexture);
		row.StreamManaged = false;
		snapshot.Rows.push_back(std::move(row));
	}
	for (const auto& [cacheKey, texture] : m_pathTextures)
	{
		if (!texture.Texture)
		{
			continue;
		}
		snapshot.Rows.push_back(BuildDiagnosticsRow(
		    texture.Texture,
		    TextureDiagnosticsKind::Scene,
		    std::filesystem::path(cacheKey).generic_string(),
		    resolvePreviewTexture));
	}
	std::sort(
	    snapshot.Rows.begin(),
	    snapshot.Rows.end(),
	    [](const TextureDiagnosticsRow& lhs, const TextureDiagnosticsRow& rhs) noexcept
	    {
		    if (lhs.Kind != rhs.Kind)
		    {
			    return static_cast<std::uint8_t>(lhs.Kind) < static_cast<std::uint8_t>(rhs.Kind);
		    }
		    return lhs.Key < rhs.Key;
	    });
	return snapshot;
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

		RendererTexture texture = CreateTexture(source->Path, decodedTexture, commandList);
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
		Diagnostics::Fatal(g_textureCacheLogger, __FILE__, __LINE__, "Texture generation could not enter residency.");
	if (m_taskScope == nullptr)
		Diagnostics::Fatal(g_textureCacheLogger, __FILE__, __LINE__, "Texture cache has no task scope.");

	auto payload = std::make_shared<TextureLoadPayload>();
	TaskGraphBuilder graph;
	const TaskNodeHandle read = graph.Add(
	    TaskDesc{.Name = TaskName("Read cooked texture generation"), .Lane = TaskLane::BlockingIo},
	    [path = source.Path, payload](TaskExecutionContext& context)
	    {
		    if (context.IsCancellationRequested())
		    {
			    return TaskResult::Cancelled("Texture read cancelled.");
		    }
		    payload->File = CookedTextureLoader::Read(path);
		    return TaskResult::Success();
	    });
	graph.ContinueWith(
	    read,
	    TaskDesc{.Name = TaskName("Decode cooked texture generation"), .Lane = TaskLane::Background},
	    [payload](TaskExecutionContext& context)
	    {
		    if (context.IsCancellationRequested())
		    {
			    payload->File = {};
			    return TaskResult::Cancelled("Texture decode cancelled.");
		    }
		    payload->Texture = CookedTextureLoader::Decode(payload->File);
		    payload->File = {};
		    return TaskResult::Success();
	    });

	TextureRequest request;
	request.Source = source;
	request.Generation = *handle;
	request.Payload = payload;
	request.Execution = m_taskExecutor.Launch(*m_taskScope, graph.Compile());
	if (!request.Execution.IsValid())
		Diagnostics::Fatal(g_textureCacheLogger, __FILE__, __LINE__, "Texture loading task graph launch failed.");
	m_requests.push_back(std::move(request));
}

void TextureCache::SynchronizeSceneTextures(const RenderTextureTable& textures)
{
	if (textures.Generation == 0)
		Diagnostics::Fatal(g_textureCacheLogger, __FILE__, __LINE__, "Scene texture table has no generation identity.");
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
			Diagnostics::Fatal(g_textureCacheLogger, __FILE__, __LINE__, "Texture task graph produced no decoded payload.");

		if (!m_residency.BeginDecoding(request.Generation))
			Diagnostics::Fatal(g_textureCacheLogger, __FILE__, __LINE__, "Texture generation could not enter decoding.");
		const std::uint64_t decodedBytes = CalculateTexturePayloadBytes(request.Payload->Texture.Upload);
		if (!m_residency.PublishReadyForUpload(request.Generation, decodedBytes, decodedBytes))
			Diagnostics::Fatal(g_textureCacheLogger, __FILE__, __LINE__, "Texture generation exceeded residency capacity.");

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
		        return !request.Execution.IsValid() && !request.Decoded && !request.Uploaded &&
		               state == AssetResidencyState::Retired;
	        }),
	    m_requests.end());
}

void TextureCache::UploadReadyTextures(RenderCommandList& commandList, std::vector<RhiResourceHandle>& uploadedResources)
{
	for (TextureRequest& request : m_requests)
	{
		if (!request.Wanted || !request.Decoded || request.Uploaded ||
		    m_residency.GetState(request.Generation) != AssetResidencyState::ReadyForUpload || !m_residency.BeginUpload(request.Generation))
		{
			continue;
		}

		request.Uploaded = CreateTexture(request.Source.Path, *request.Decoded, commandList);

		request.Decoded.reset();
		request.UploadedResource = m_resourceService.GetResourceHandle(request.Uploaded->Resource);
		uploadedResources.push_back(request.UploadedResource);
	}
}

RendererTexture TextureCache::CreateTexture(
    const std::filesystem::path& texturePath,
    LoadedTextureData& loadedTexture,
    RenderCommandList& commandList) const
{
	const RhiTextureUploadDesc& textureUpload = loadedTexture.Upload;
	const std::wstring debugName = texturePath.filename().wstring();
	const RhiTextureResourceDesc resourceDesc{
	    .Width = textureUpload.Width,
	    .Height = textureUpload.Height,
	    .Format = textureUpload.Format,
	    .MipLevels = textureUpload.GetMipCount(),
	    .ArraySize = textureUpload.GetArraySize(),
	    .Dimension = textureUpload.Dimension};
	RhiOwnedResourceHandle resource = m_resourceService.CreateTextureResource(
	    resourceDesc,
	    ResourceState::CopyDest,
	    RhiMemoryCategory::Texture,
	    RhiMemoryResidencyClass::DeviceLocal,
	    debugName);
	if (!resource)
		Diagnostics::Fatal(
		    g_textureCacheLogger,
		    __FILE__,
		    __LINE__,
		    std::format("Texture resource creation failed for '{}'.", texturePath.string()));

	if (!m_uploadService.UploadTexture(commandList, resource, textureUpload, ResourceState::ShaderResource, debugName))
	{
		m_resourceService.ReleaseOwnedResource(resource);
		Diagnostics::Fatal(
		    g_textureCacheLogger,
		    __FILE__,
		    __LINE__,
		    std::format("Texture upload failed for '{}'.", texturePath.string()));
	}

	const RhiResourceHandle nativeResource = m_resourceService.GetResourceHandle(resource);
	RhiResourceViewHandle shaderResourceView = m_descriptorService.CreateResourceView(
	    RhiResourceViewDesc::TextureShaderResource(
	        nativeResource,
	        textureUpload.Format,
	        RhiTextureViewRange{
	            .MostDetailedMip = 0,
	            .MipCount = textureUpload.GetMipCount(),
	            .FirstArraySlice = 0,
	            .ArraySize = textureUpload.GetArraySize()},
	        textureUpload.Dimension));
	if (!shaderResourceView)
	{
		m_resourceService.ReleaseOwnedResource(resource);
		Diagnostics::Fatal(
		    g_textureCacheLogger,
		    __FILE__,
		    __LINE__,
		    std::format("Texture shader-resource view creation failed for '{}'.", texturePath.string()));
	}

	return RendererTexture{
	    .Resource = resource,
	    .ShaderResourceView = shaderResourceView,
	    .Width = textureUpload.Width,
	    .Height = textureUpload.Height,
	    .ArraySize = textureUpload.ArraySize,
	    .Dimension = textureUpload.Dimension,
	    .Format = textureUpload.Format,
	    .FormatIntent = loadedTexture.FormatIntent,
	    .MipCount = textureUpload.GetMipCount(),
	    .EstimatedByteSize = CalculateTexturePayloadBytes(textureUpload)};
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
	    {
		    return candidate.Source.CacheKey == cacheKey && candidate.Generation.Generation == generation;
	    });
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
			ReleaseTexture(*request.Uploaded);
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
	        {
		        return !request.Execution.IsValid() && !request.Decoded && !request.Uploaded;
	        }),
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
		Diagnostics::Fatal(g_textureCacheLogger, __FILE__, __LINE__, "Resident texture could not enter eviction.");
	ReleaseTexture(texture.Texture);
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

std::optional<TextureCache::ResolvedTexturePath> TextureCache::ResolveTexturePath(
    const std::filesystem::path& texturePath) const noexcept
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

void TextureCache::ReleaseTexture(RendererTexture& texture) noexcept
{
	if (texture.ShaderResourceView)
	{
		m_descriptorService.ReleaseResourceView(texture.ShaderResourceView);
	}
	if (texture.Resource)
	{
		m_resourceService.ReleaseOwnedResource(texture.Resource);
	}
	texture = {};
}

TextureDiagnosticsRow TextureCache::BuildDiagnosticsRow(
    const RendererTexture& texture,
    TextureDiagnosticsKind kind,
    const std::string& key,
    const PreviewTextureResolver& resolvePreviewTexture) const
{
	TextureDiagnosticsRow row;
	row.Key = key;
	row.Kind = kind;
	row.Dimension = texture.Dimension;
	row.FormatIntent = texture.FormatIntent;
	row.ResidencyState = TextureDiagnosticsResidencyState::Resident;
	row.Width = texture.Width;
	row.Height = texture.Height;
	row.ArraySize = texture.ArraySize;
	row.Format = PixelFormatName(texture.Format);
	row.MipCount = texture.MipCount;
	row.EstimatedByteSize = texture.EstimatedByteSize;
	const std::uint64_t nativeTextureId = m_descriptorService.GetResourceViewGpuHandle(texture.ShaderResourceView).Value;
	row.PreviewTexture = resolvePreviewTexture ? resolvePreviewTexture(nativeTextureId) : EditorTextureHandle{};
	row.Loaded = static_cast<bool>(texture);
	row.StreamManaged = true;
	return row;
}

std::uint64_t TextureCache::CalculateTexturePayloadBytes(const RhiTextureUploadDesc& textureUpload) noexcept
{
	std::uint64_t byteCount = 0;
	for (const RhiTextureArraySliceUploadData& arraySlice : textureUpload.ArraySlices)
	{
		for (const RhiTextureMipUploadData& mipLevel : arraySlice.MipLevels)
		{
			byteCount += mipLevel.Data.size();
		}
	}

	return byteCount;
}

std::uint64_t TextureCache::MakeAssetKey(const TextureKey& cacheKey)
{
	const std::uint64_t value = static_cast<std::uint64_t>(std::hash<TextureKey>{}(cacheKey));
	if (value == 0)
		Diagnostics::Fatal(g_textureCacheLogger, __FILE__, __LINE__, "Texture path hashes to the null asset identity.");
	return value;
}
