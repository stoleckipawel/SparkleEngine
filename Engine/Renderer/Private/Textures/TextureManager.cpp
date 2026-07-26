#include "../PCH.h"

#include "Textures/TextureManager.h"

#include "Assets/Cooked/CookedTextureReference.h"
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

static const auto g_textureManagerLogger = Logging::GetOrCreateLogger("Renderer.TextureManager");

class TextureManagerOperations final
{
  public:
	static std::uint64_t CalculateTexturePayloadBytes(const RhiTextureUploadDesc& textureUpload) noexcept
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

	static std::uint64_t MakeAssetKey(const std::wstring& cacheKey) noexcept
	{
		const std::uint64_t value = static_cast<std::uint64_t>(std::hash<std::wstring>{}(cacheKey));
		return value != 0 ? value : 1;
	}
};

TextureManager::TextureManager(
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
    m_taskScope(std::make_unique<TaskScope>(
        TaskScopeDesc{TaskScopeKind::AssetGeneration, "Renderer texture generations"},
        &applicationScope))
{
}

TextureManager::~TextureManager() noexcept
{
	if (m_taskScope != nullptr)
	{
		m_taskScope->Cancel();
		(void)m_taskScope->JoinFor(std::chrono::milliseconds::max());
	}
	UnloadAll();
}

std::vector<RhiResourceHandle> TextureManager::LoadSceneTextures(
    const RenderTextureTable& textures,
    RenderCommandList& commandList)
{
	RequestDefaults();
	for (const RenderTextureAsset& texture : textures.Assets)
	{
		const std::optional<ResolvedTexturePath> source = ResolveTexturePath(texture.Path);
		if (source)
		{
			RequestTexture(*source, textures.Generation != 0 ? textures.Generation : 1);
		}
	}

	ConsumeCompletedRequests();
	std::vector<RhiResourceHandle> uploadedResources;
	UploadReadyTextures(commandList, uploadedResources);
	return uploadedResources;
}

bool TextureManager::HasPendingSceneTextureUploads(const RenderTextureTable& textures) const noexcept
{
	(void)textures;
	return std::any_of(
	    m_requests.begin(),
	    m_requests.end(),
	    [this](const TextureRequest& request) noexcept
	    {
		    return request.Wanted &&
		           request.Decoded.has_value() &&
		           m_residency.GetState(request.Generation) ==
		               AssetResidencyState::ReadyForUpload;
	    });
}

void TextureManager::RecordUploadSubmission(RhiSubmissionToken token) noexcept
{
	if (!token.IsValid())
	{
		return;
	}

	for (TextureRequest& request : m_requests)
	{
		if (request.Uploaded && !request.UploadSubmitted)
		{
			request.UploadSubmitted =
			    m_residency.RecordUploadSubmission(
			        request.Generation,
			        token,
			        request.Uploaded->EstimatedByteSize);
		}
	}
}

void TextureManager::PollResidency() noexcept
{
	m_residency.Poll(m_submissions);
	ActivateResidentRequests();
}

void TextureManager::UnloadSceneTextures() noexcept
{
	for (TextureRequest& request : m_requests)
	{
		if (!m_defaultPathTextureKeys.contains(request.Source.CacheKey))
		{
			request.Wanted = false;
			(void)m_residency.Cancel(request.Generation);
		}
	}

	for (auto texture = m_pathTextures.begin(); texture != m_pathTextures.end();)
	{
		if (m_defaultPathTextureKeys.contains(texture->first))
		{
			++texture;
			continue;
		}
		RetireActiveTexture(texture->second);
		texture = m_pathTextures.erase(texture);
	}
}

void TextureManager::UnloadAll() noexcept
{
	for (TextureRequest& request : m_requests)
	{
		request.Wanted = false;
		(void)m_residency.Cancel(request.Generation);
		if (request.Uploaded)
		{
			ReleaseTexture(*request.Uploaded);
			request.Uploaded.reset();
		}
	}
	m_requests.clear();

	for (auto& [cacheKey, texture] : m_pathTextures)
	{
		(void)cacheKey;
		RetireActiveTexture(texture);
	}
	m_pathTextures.clear();
	m_defaultPathTextureKeys.clear();
	m_textureSlotKeys = {};
	m_defaultsRequested = false;
}

const RendererTexture* TextureManager::GetTexture(TextureId id) const noexcept
{
	const std::size_t index = static_cast<std::size_t>(id);
	if (index >= kTextureCount || m_textureSlotKeys[index].empty())
	{
		return nullptr;
	}
	const auto texture = m_pathTextures.find(m_textureSlotKeys[index]);
	return texture != m_pathTextures.end() ? &texture->second.Texture : nullptr;
}

const RendererTexture* TextureManager::ResolveDefaultSkyTexture() const noexcept
{
	if (const RendererTexture* skyTexture = GetTexture(TextureId::DefaultSky))
	{
		return skyTexture;
	}
	return GetTexture(TextureId::Checker);
}

const RendererTexture* TextureManager::GetSceneTexture(const std::filesystem::path& texturePath) const noexcept
{
	return FindPathTexture(texturePath);
}

const RendererTexture* TextureManager::ResolveTextureReferenceOrDefault(
    const Assets::CookedTextureReference* textureReference,
    DefaultTexture fallbackType) const
{
	if (textureReference != nullptr && textureReference->IsValid())
	{
		if (const RendererTexture* texture = GetSceneTexture(textureReference->texturePath))
		{
			return texture;
		}
	}

	if (const RendererTexture* texture = FindPathTexture(DefaultTextures::GetPath(fallbackType)))
	{
		return texture;
	}
	if (const RendererTexture* texture = FindPathTexture(DefaultTextures::GetPath(DefaultTexture::Checkerboard)))
	{
		return texture;
	}
	return GetTexture(TextureId::Checker);
}

TextureDiagnosticsSnapshot TextureManager::CaptureDiagnosticsSnapshot(
    const PreviewTextureResolver& resolvePreviewTexture) const
{
	TextureDiagnosticsSnapshot snapshot;
	snapshot.Rows.reserve(m_pathTextures.size());
	for (const auto& [cacheKey, texture] : m_pathTextures)
	{
		if (!texture.Texture)
		{
			continue;
		}
		const TextureDiagnosticsKind kind =
		    m_defaultPathTextureKeys.contains(cacheKey)
		        ? TextureDiagnosticsKind::DefaultPath
		        : TextureDiagnosticsKind::Scene;
		snapshot.Rows.push_back(
		    BuildDiagnosticsRow(
		        texture.Texture,
		        kind,
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
			    return static_cast<std::uint8_t>(lhs.Kind) <
			           static_cast<std::uint8_t>(rhs.Kind);
		    }
		    return lhs.Key < rhs.Key;
	    });
	return snapshot;
}

void TextureManager::RequestDefaults()
{
	if (m_defaultsRequested)
	{
		return;
	}

	for (std::uint8_t index = 0; index < static_cast<std::uint8_t>(DefaultTexture::Count); ++index)
	{
		const DefaultTexture type = static_cast<DefaultTexture>(index);
		RegisterDefaultPathTexture(DefaultTextures::GetPath(type));
		const std::optional<ResolvedTexturePath> source = ResolveTexturePath(DefaultTextures::GetPath(type));
		if (source)
		{
			RequestTexture(*source, 1);
		}
	}

	const std::optional<ResolvedTexturePath> checker =
	    ResolveTexturePath(DefaultTextures::GetPath(DefaultTexture::Checkerboard));
	const std::optional<ResolvedTexturePath> sky =
	    ResolveTexturePath(DefaultTextures::GetPath(DefaultTexture::Sky));
	if (checker)
	{
		m_textureSlotKeys[static_cast<std::size_t>(TextureId::Checker)] = checker->CacheKey;
	}
	if (sky)
	{
		m_textureSlotKeys[static_cast<std::size_t>(TextureId::DefaultSky)] = sky->CacheKey;
	}
	m_defaultsRequested = true;
}

void TextureManager::RequestTexture(
    const ResolvedTexturePath& source,
    std::uint32_t generation)
{
	const auto active = m_pathTextures.find(source.CacheKey);
	if (active != m_pathTextures.end() &&
	    active->second.Generation.Generation == generation)
	{
		return;
	}
	if (FindRequest(source.CacheKey, generation) != nullptr)
	{
		return;
	}

	const std::optional<AssetGenerationHandle> handle =
	    m_residency.BeginGeneration(
	        TextureManagerOperations::MakeAssetKey(source.CacheKey),
	        generation);
	if (!handle || m_taskScope == nullptr)
	{
		return;
	}

	auto result = std::make_shared<TextureLoadResult>();
	TaskGraphBuilder graph;
	const TaskNodeHandle read = graph.Add(
	    TaskDesc{
	        .Name = TaskName("Read cooked texture generation"),
	        .Lane = TaskLane::BlockingIo},
	    [path = source.Path, result](TaskExecutionContext& context)
	    {
		    if (context.IsCancellationRequested())
		    {
			    return TaskResult::Cancelled("Texture read cancelled.");
		    }
		    return CookedTextureLoader::TryRead(path, result->File, result->ErrorMessage)
		               ? TaskResult::Success()
		               : TaskResult::Failure(result->ErrorMessage);
	    });
	graph.ContinueWith(
	    read,
	    TaskDesc{
	        .Name = TaskName("Decode cooked texture generation"),
	        .Lane = TaskLane::Background},
	    [result](TaskExecutionContext& context)
	    {
		    if (context.IsCancellationRequested())
		    {
			    result->File = {};
			    return TaskResult::Cancelled("Texture decode cancelled.");
		    }
		    const bool decoded =
		        CookedTextureLoader::TryDecode(
		            result->File,
		            result->Texture,
		            result->ErrorMessage);
		    result->File = {};
		    return decoded
		               ? TaskResult::Success()
		               : TaskResult::Failure(result->ErrorMessage);
	    });

	TextureRequest request;
	request.Source = source;
	request.Generation = *handle;
	request.Result = result;
	request.Execution =
	    m_taskExecutor.Launch(*m_taskScope, graph.Compile());
	m_requests.push_back(std::move(request));
}

void TextureManager::ConsumeCompletedRequests() noexcept
{
	for (TextureRequest& request : m_requests)
	{
		if (!request.Execution.IsValid() || !request.Execution.IsSettled())
		{
			continue;
		}

		const TaskExecutionStatus executionStatus = request.Execution.GetStatus();
		request.Execution = {};
		if (!request.Wanted || executionStatus == TaskExecutionStatus::Cancelled)
		{
			(void)m_residency.Cancel(request.Generation);
			request.Result.reset();
			continue;
		}
		if (executionStatus != TaskExecutionStatus::Succeeded ||
		    request.Result == nullptr ||
		    !request.Result->Texture.IsValid())
		{
			(void)m_residency.MarkFailed(request.Generation);
			if (request.Result != nullptr && !request.Result->ErrorMessage.empty())
			{
				SPDLOG_LOGGER_WARN(
				    g_textureManagerLogger,
				    "Texture generation {} for '{}' failed: {}",
				    request.Generation.Generation,
				    request.Source.Path.string(),
				    request.Result->ErrorMessage);
			}
			request.Result.reset();
			continue;
		}

		(void)m_residency.BeginDecoding(request.Generation);
		const std::uint64_t decodedBytes =
		    TextureManagerOperations::CalculateTexturePayloadBytes(
		        request.Result->Texture.Upload);
		if (!m_residency.PublishReadyForUpload(
		        request.Generation,
		        decodedBytes,
		        decodedBytes))
		{
			(void)m_residency.MarkFailed(request.Generation);
			request.Result.reset();
			continue;
		}

		request.Decoded.emplace(std::move(request.Result->Texture));
		request.Result.reset();
	}

	m_requests.erase(
	    std::remove_if(
	        m_requests.begin(),
	        m_requests.end(),
	        [this](const TextureRequest& request) noexcept
	        {
		        const AssetResidencyState state =
		            m_residency.GetState(request.Generation);
		        return !request.Execution.IsValid() &&
		               !request.Decoded &&
		               !request.Uploaded &&
		               (state == AssetResidencyState::Failed ||
		                state == AssetResidencyState::Retired);
	        }),
	    m_requests.end());
}

void TextureManager::UploadReadyTextures(
    RenderCommandList& commandList,
    std::vector<RhiResourceHandle>& uploadedResources)
{
	for (TextureRequest& request : m_requests)
	{
		if (!request.Wanted || !request.Decoded ||
		    request.Uploaded ||
		    m_residency.GetState(request.Generation) != AssetResidencyState::ReadyForUpload ||
		    !m_residency.BeginUpload(request.Generation))
		{
			continue;
		}

		RhiResourceHandle uploadedResource;
		request.Uploaded =
		    CreateTexture(
		        request.Source.Path,
		        *request.Decoded,
		        commandList,
		        uploadedResource);
		if (!request.Uploaded)
		{
			(void)m_residency.MarkFailed(request.Generation);
			request.Decoded.reset();
			continue;
		}

		request.Decoded.reset();
		request.UploadedResource = uploadedResource;
		uploadedResources.push_back(uploadedResource);
	}
}

std::optional<RendererTexture> TextureManager::CreateTexture(
    const std::filesystem::path& texturePath,
    LoadedTextureData& loadedTexture,
    RenderCommandList& commandList,
    RhiResourceHandle& outUploadedResource) const
{
	outUploadedResource = {};
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
	{
		return std::nullopt;
	}

	if (!m_uploadService.UploadTexture(
	        commandList,
	        resource,
	        textureUpload,
	        ResourceState::ShaderResource,
	        debugName))
	{
		m_resourceService.ReleaseOwnedResource(resource);
		return std::nullopt;
	}

	const RhiResourceHandle nativeResource =
	    m_resourceService.GetResourceHandle(resource);
	RhiResourceViewHandle shaderResourceView =
	    m_descriptorService.CreateResourceView(
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
		return std::nullopt;
	}

	outUploadedResource = nativeResource;
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
	    .EstimatedByteSize =
	        TextureManagerOperations::CalculateTexturePayloadBytes(textureUpload)};
}

TextureManager::TextureRequest* TextureManager::FindRequest(
    const TextureCacheKey& cacheKey,
    std::uint32_t generation) noexcept
{
	return const_cast<TextureRequest*>(
	    std::as_const(*this).FindRequest(cacheKey, generation));
}

const TextureManager::TextureRequest* TextureManager::FindRequest(
    const TextureCacheKey& cacheKey,
    std::uint32_t generation) const noexcept
{
	const auto request = std::find_if(
	    m_requests.begin(),
	    m_requests.end(),
	    [&cacheKey, generation](const TextureRequest& candidate) noexcept
	    {
		    return candidate.Source.CacheKey == cacheKey &&
		           candidate.Generation.Generation == generation;
	    });
	return request != m_requests.end() ? &*request : nullptr;
}

void TextureManager::ActivateResidentRequests() noexcept
{
	for (TextureRequest& request : m_requests)
	{
		if (!request.Uploaded)
		{
			continue;
		}

		const AssetResidencyState state =
		    m_residency.GetState(request.Generation);
		if (!request.Wanted &&
		    (state == AssetResidencyState::Retired ||
		     state == AssetResidencyState::Failed))
		{
			ReleaseTexture(*request.Uploaded);
			request.Uploaded.reset();
			continue;
		}
		if (!request.Wanted ||
		    m_residency.GetState(request.Generation) != AssetResidencyState::Resident)
		{
			continue;
		}

		auto active = m_pathTextures.find(request.Source.CacheKey);
		if (active != m_pathTextures.end() &&
		    active->second.Generation.Generation >
		        request.Generation.Generation)
		{
			ActiveTexture staleTexture{
			    .Generation = request.Generation,
			    .Texture = std::move(*request.Uploaded)};
			RetireActiveTexture(staleTexture);
			request.Uploaded.reset();
			continue;
		}
		if (active != m_pathTextures.end())
		{
			RetireActiveTexture(active->second);
			active->second = ActiveTexture{
			    .Generation = request.Generation,
			    .Texture = std::move(*request.Uploaded)};
		}
		else
		{
			m_pathTextures.emplace(
			    request.Source.CacheKey,
			    ActiveTexture{
			        .Generation = request.Generation,
			        .Texture = std::move(*request.Uploaded)});
		}
		request.Uploaded.reset();
	}

	m_requests.erase(
	    std::remove_if(
	        m_requests.begin(),
	        m_requests.end(),
	        [](const TextureRequest& request) noexcept
	        {
		        return !request.Execution.IsValid() &&
		               !request.Decoded &&
		               !request.Uploaded;
	        }),
	    m_requests.end());
}

void TextureManager::RetireActiveTexture(ActiveTexture& texture) noexcept
{
	const RhiSubmissionState lastUse = CaptureLastSubmittedState();
	(void)m_residency.BeginEviction(texture.Generation, lastUse);
	ReleaseTexture(texture.Texture);
}

RhiSubmissionState TextureManager::CaptureLastSubmittedState() const noexcept
{
	RhiSubmissionState state;
	for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
	{
		state.MarkUsed(
		    m_submissions.GetLastSubmittedToken(
		        static_cast<ERhiQueueType>(queueIndex)));
	}
	return state;
}

void TextureManager::RegisterDefaultPathTexture(
    const std::filesystem::path& texturePath)
{
	const std::optional<ResolvedTexturePath> resolved =
	    ResolveTexturePath(texturePath);
	if (resolved)
	{
		m_defaultPathTextureKeys.insert(resolved->CacheKey);
	}
}

const RendererTexture* TextureManager::FindPathTexture(
    const std::filesystem::path& texturePath) const noexcept
{
	const std::optional<ResolvedTexturePath> resolved =
	    ResolveTexturePath(texturePath);
	if (!resolved)
	{
		return nullptr;
	}
	const auto texture = m_pathTextures.find(resolved->CacheKey);
	return texture != m_pathTextures.end()
	           ? &texture->second.Texture
	           : nullptr;
}

std::optional<TextureManager::ResolvedTexturePath>
TextureManager::ResolveTexturePath(
    const std::filesystem::path& texturePath) const noexcept
{
	const auto resolvedPath =
	    Filesystem::ResolveAssetPathNormalized(
	        texturePath,
	        AssetType::Texture);
	if (!resolvedPath)
	{
		return std::nullopt;
	}
	TextureCacheKey cacheKey = Paths::MakePathKey(*resolvedPath);
	if (cacheKey.empty())
	{
		return std::nullopt;
	}
	return ResolvedTexturePath{
	    .Path = *resolvedPath,
	    .CacheKey = std::move(cacheKey)};
}

void TextureManager::ReleaseTexture(RendererTexture& texture) noexcept
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

TextureDiagnosticsRow TextureManager::BuildDiagnosticsRow(
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
	const std::uint64_t nativeTextureId =
	    m_descriptorService.GetResourceViewGpuHandle(
	        texture.ShaderResourceView)
	        .Value;
	row.PreviewTexture =
	    resolvePreviewTexture
	        ? resolvePreviewTexture(nativeTextureId)
	        : EditorTextureHandle{};
	row.Loaded = static_cast<bool>(texture);
	row.StreamManaged = true;
	return row;
}
