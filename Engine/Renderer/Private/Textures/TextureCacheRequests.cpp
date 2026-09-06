#include "PCH.h"
#include "Textures/TextureCache.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "RHI/Public/Commands/RenderCommandList.h"
#include "RHI/Public/Resources/RhiResourceService.h"
#include "Textures/CookedTextureLoader.h"

#include <algorithm>
#include <format>
#include <utility>

static const auto g_textureCacheLogger = Logging::GetOrCreateLogger("Renderer.TextureCache");

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
		m_defaultTextures.emplace(source->CacheKey, texture);
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

		QueueTextureRetirement(texture->second, nextBindingRevision);
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
