#include "PCH.h"

#include "Textures/TextureManager.h"

#include "Assets/Cooked/CookedTextureReference.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Paths/PathUtils.h"
#include "RHI/Public/Commands/RenderCommandList.h"
#include "RHI/Public/Descriptors/RhiDescriptorService.h"
#include "RHI/Public/Resources/RhiResourceService.h"
#include "RHI/Public/Resources/RhiUploadService.h"
#include "Textures/CookedTextureLoader.h"

#include <algorithm>
#include <format>
#include <utility>

static const auto g_textureManagerLogger = Logging::GetOrCreateLogger("Renderer.TextureManager");

namespace
{
	std::uint64_t CalculateTexturePayloadBytes(const RhiTextureUploadDesc& textureUpload) noexcept
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
}

std::optional<RendererTexture> TextureManager::CreateTextureFromPath(
    const std::filesystem::path& texturePath,
    RenderCommandList& commandList,
    std::vector<RhiResourceHandle>& uploadedResources) const
{
	LoadedTextureData loadedTexture = CookedTextureLoader::Load(texturePath);
	if (!loadedTexture.IsValid())
	{
		return std::nullopt;
	}

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
		SPDLOG_LOGGER_WARN(
		    g_textureManagerLogger,
		    "TextureManager::CreateTextureFromPath: Resource creation failed for '{}' ({}x{}, mips={}, slices={}, format={}).",
		    texturePath.string(),
		    resourceDesc.Width,
		    resourceDesc.Height,
		    resourceDesc.MipLevels,
		    resourceDesc.ArraySize,
		    static_cast<std::uint32_t>(resourceDesc.Format));
		return std::nullopt;
	}

	if (!m_uploadService.UploadTexture(
	        commandList,
	        resource,
	        textureUpload,
	        ResourceState::ShaderResource,
	        debugName))
	{
		SPDLOG_LOGGER_WARN(
		    g_textureManagerLogger,
		    "TextureManager::CreateTextureFromPath: Upload recording failed for '{}' on the {} queue.",
		    texturePath.string(),
		    RhiQueueTypeToString(commandList.GetQueueType()));
		m_resourceService.ReleaseOwnedResource(resource);
		return std::nullopt;
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
		SPDLOG_LOGGER_WARN(
		    g_textureManagerLogger,
		    "TextureManager::CreateTextureFromPath: Shader-resource view creation failed for '{}'.",
		    texturePath.string());
		m_resourceService.ReleaseOwnedResource(resource);
		return std::nullopt;
	}
	uploadedResources.push_back(nativeResource);

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

TextureManager::TextureManager(
    RhiResourceService& resourceService,
    RhiDescriptorService& descriptorService,
    RhiUploadService& uploadService) noexcept :
    m_resourceService(resourceService),
    m_descriptorService(descriptorService),
    m_uploadService(uploadService)
{
}

TextureManager::~TextureManager() noexcept
{
	UnloadAll();
}

void TextureManager::LoadDefaults(RenderCommandList& commandList, std::vector<RhiResourceHandle>& uploadedResources)
{
	if (m_defaultsLoaded)
	{
		return;
	}
	if (GetTexture(TextureId::Checker) == nullptr)
	{
		LoadTexture(TextureId::Checker, DefaultTextures::GetPath(DefaultTexture::Checkerboard), commandList, uploadedResources);
	}
	if (GetTexture(TextureId::DefaultSky) == nullptr)
	{
		LoadTexture(TextureId::DefaultSky, DefaultTextures::GetPath(DefaultTexture::Sky), commandList, uploadedResources);
	}
	LoadDefaultTextures(commandList, uploadedResources);
	m_defaultsLoaded = GetTexture(TextureId::Checker) != nullptr && GetTexture(TextureId::DefaultSky) != nullptr;
}

std::vector<RhiResourceHandle> TextureManager::LoadSceneTextures(
	const TextureSnapshot& textureSnapshot,
	RenderCommandList& commandList)
{
	std::vector<RhiResourceHandle> uploadedResources;
	LoadDefaults(commandList, uploadedResources);
	for (const std::filesystem::path& texturePath : textureSnapshot.texturePaths)
	{
		LoadFromPath(texturePath, commandList, uploadedResources);
	}
	return uploadedResources;
}

bool TextureManager::HasPendingSceneTextureUploads(const TextureSnapshot& textureSnapshot) const noexcept
{
	if (!m_defaultsLoaded)
	{
		return true;
	}
	for (const std::filesystem::path& texturePath : textureSnapshot.texturePaths)
	{
		const std::optional<ResolvedTexturePath> resolved = ResolveTexturePath(texturePath);
		if (resolved && !m_pathTextures.contains(resolved->CacheKey))
		{
			return true;
		}
	}
	return false;
}

void TextureManager::LoadTexture(
	TextureId id,
	const std::filesystem::path& relativePath,
	RenderCommandList& commandList,
	std::vector<RhiResourceHandle>& uploadedResources)
{
	const auto index = static_cast<std::size_t>(id);
	if (index >= kTextureCount)
	{
		SPDLOG_LOGGER_ERROR(g_textureManagerLogger, "{}", std::format("TextureManager::LoadTexture: Invalid texture ID {}", index));
		return;
	}

	if (m_textures[index])
	{
		ReleaseTexture(*m_textures[index]);
		m_textures[index].reset();
	}

	m_textures[index] = CreateTextureFromPath(relativePath, commandList, uploadedResources);
	if (!m_textures[index])
	{
		SPDLOG_LOGGER_ERROR(
		    g_textureManagerLogger,
		    "{}",
		    std::format("TextureManager::LoadTexture: Failed to load '{}' into slot {}", relativePath.string(), index));
		return;
	}
}

RendererTexture* TextureManager::LoadFromPath(
	const std::filesystem::path& texturePath,
	RenderCommandList& commandList,
	std::vector<RhiResourceHandle>& uploadedResources)
{
	const std::optional<ResolvedTexturePath> resolved = ResolveTexturePath(texturePath);
	if (!resolved)
	{
		SPDLOG_LOGGER_WARN(
		    g_textureManagerLogger,
		    "{}",
		    std::format("TextureManager::LoadFromPath: Failed to resolve '{}'", texturePath.string()));
		return nullptr;
	}

	if (auto it = m_pathTextures.find(resolved->CacheKey); it != m_pathTextures.end())
	{
		return &it->second;
	}

	std::optional<RendererTexture> texture = CreateTextureFromPath(resolved->Path, commandList, uploadedResources);
	if (!texture)
	{
		SPDLOG_LOGGER_WARN(
		    g_textureManagerLogger,
		    "{}",
		    std::format("TextureManager::LoadFromPath: Failed to create texture for '{}'", resolved->Path.string()));
		return nullptr;
	}

	auto [inserted, wasInserted] = m_pathTextures.emplace(resolved->CacheKey, std::move(*texture));
	return wasInserted ? &inserted->second : nullptr;
}
void TextureManager::UnloadSceneTextures() noexcept
{
	for (auto it = m_pathTextures.begin(); it != m_pathTextures.end();)
	{
		if (m_defaultPathTextureKeys.contains(it->first))
		{
			++it;
			continue;
		}

		ReleaseTexture(it->second);
		it = m_pathTextures.erase(it);
	}
}

void TextureManager::UnloadAll() noexcept
{
	for (auto& entry : m_pathTextures)
	{
		ReleaseTexture(entry.second);
	}
	m_pathTextures.clear();
	m_defaultPathTextureKeys.clear();
	for (auto& texture : m_textures)
	{
		if (texture)
		{
			ReleaseTexture(*texture);
		}
		texture.reset();
	}
	m_defaultsLoaded = false;
}

const RendererTexture* TextureManager::GetTexture(TextureId id) const noexcept
{
	const auto index = static_cast<std::size_t>(id);
	return index < kTextureCount && m_textures[index] ? &*m_textures[index] : nullptr;
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
	if (textureReference && textureReference->IsValid())
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

	SPDLOG_LOGGER_WARN(
	    g_textureManagerLogger,
	    "{}",
	    std::format("TextureManager: Falling back to checkerboard for {} default texture", DefaultTextures::GetName(fallbackType)));
	if (const RendererTexture* texture = FindPathTexture(DefaultTextures::GetPath(DefaultTexture::Checkerboard)))
	{
		return texture;
	}

	return GetTexture(TextureId::Checker);
}

TextureDiagnosticsSnapshot TextureManager::CaptureDiagnosticsSnapshot() const
{
	TextureDiagnosticsSnapshot snapshot;
	snapshot.Rows.reserve(kTextureCount + m_pathTextures.size());

	for (std::size_t index = 0; index < kTextureCount; ++index)
	{
		const std::optional<RendererTexture>& texture = m_textures[index];
		if (!texture)
		{
			continue;
		}

		snapshot.Rows.push_back(BuildDiagnosticsRow(*texture, TextureDiagnosticsKind::DefaultSlot, std::format("slot:{}", index)));
	}

	for (const auto& [cacheKey, texture] : m_pathTextures)
	{
		if (!texture)
		{
			continue;
		}

		const bool defaultOrFallback = m_defaultPathTextureKeys.contains(cacheKey);
		const TextureDiagnosticsKind kind = defaultOrFallback ? TextureDiagnosticsKind::DefaultPath : TextureDiagnosticsKind::Scene;
		snapshot.Rows.push_back(BuildDiagnosticsRow(texture, kind, std::filesystem::path(cacheKey).generic_string()));
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

void TextureManager::LoadDefaultTextures(
	RenderCommandList& commandList,
	std::vector<RhiResourceHandle>& uploadedResources)
{
	for (std::uint8_t index = 0; index < static_cast<std::uint8_t>(DefaultTexture::Count); ++index)
	{
		const auto type = static_cast<DefaultTexture>(index);
		RegisterDefaultPathTexture(DefaultTextures::GetPath(type));
		if (!LoadFromPath(DefaultTextures::GetPath(type), commandList, uploadedResources))
		{
			SPDLOG_LOGGER_WARN(
			    g_textureManagerLogger,
			    "{}",
			    std::format(
			        "TextureManager: Could not preload {} default texture; checker remains the emergency fallback",
			        DefaultTextures::GetName(type)));
		}
	}
}

void TextureManager::RegisterDefaultPathTexture(const std::filesystem::path& texturePath)
{
	const std::optional<ResolvedTexturePath> resolved = ResolveTexturePath(texturePath);
	if (!resolved)
	{
		return;
	}

	m_defaultPathTextureKeys.insert(resolved->CacheKey);
}

const RendererTexture* TextureManager::FindPathTexture(const std::filesystem::path& texturePath) const noexcept
{
	const std::optional<ResolvedTexturePath> resolved = ResolveTexturePath(texturePath);
	if (!resolved)
	{
		return nullptr;
	}

	if (auto it = m_pathTextures.find(resolved->CacheKey); it != m_pathTextures.end())
	{
		return &it->second;
	}

	return nullptr;
}

std::optional<TextureManager::ResolvedTexturePath> TextureManager::ResolveTexturePath(
    const std::filesystem::path& texturePath) const noexcept
{
	const auto resolvedPath = Filesystem::ResolveAssetPathNormalized(texturePath, AssetType::Texture);
	if (!resolvedPath)
	{
		return std::nullopt;
	}
	TextureCacheKey cacheKey = Paths::MakePathKey(*resolvedPath);
	if (cacheKey.empty())
	{
		return std::nullopt;
	}
	return ResolvedTexturePath{.Path = *resolvedPath, .CacheKey = std::move(cacheKey)};
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
    const std::string& key) const
{
	TextureDiagnosticsRow row;
	row.Key = key;
	row.Kind = kind;
	row.Dimension = texture.Dimension;
	row.FormatIntent = texture.FormatIntent;
	row.ResidencyState = texture ? TextureDiagnosticsResidencyState::Resident : TextureDiagnosticsResidencyState::Unloaded;
	row.Width = texture.Width;
	row.Height = texture.Height;
	row.ArraySize = texture.ArraySize;
	row.Format = PixelFormatName(texture.Format);
	row.MipCount = texture.MipCount;
	row.EstimatedByteSize = texture.EstimatedByteSize;
	row.GpuShaderResourceViewId = m_descriptorService.GetResourceViewGpuHandle(texture.ShaderResourceView).Value;
	row.Loaded = static_cast<bool>(texture);
	row.StreamManaged = false;
	return row;
}
