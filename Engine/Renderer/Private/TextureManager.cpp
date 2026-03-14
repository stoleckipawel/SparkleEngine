#include "PCH.h"
#include "TextureManager.h"
#include "FileSystemUtils.h"
#include "D3D12Texture.h"
#include "TextureLoader.h"
#include "Assets/AssetSystem.h"
#include "D3D12Rhi.h"
#include "D3D12DescriptorHeapManager.h"

#include <algorithm>
#include <cwctype>
#include <format>
#include <utility>

namespace
{
	TexturePayload CreateTexturePayloadFromLoadedData(const TextureLoader::Data& loadedData)
	{
		TexturePayload payload;
		payload.width = loadedData.width;
		payload.height = loadedData.height;
		payload.dxgiFormat = loadedData.dxgiPixelFormat;
		payload.formatIntent = TextureFormatIntent::Unknown;

		TextureMipLevelData baseMip;
		baseMip.width = loadedData.width;
		baseMip.height = loadedData.height;
		baseMip.rowPitch = loadedData.stride;
		baseMip.slicePitch = loadedData.slicePitch;
		baseMip.data = loadedData.data;
		payload.mipLevels.push_back(std::move(baseMip));

		return payload;
	}

	TexturePayload LoadTexturePayload(const AssetSystem& assetSystem, const std::filesystem::path& filePath)
	{
		const TextureLoader loader(assetSystem, filePath);
		return CreateTexturePayloadFromLoadedData(loader.GetData());
	}
}  // namespace

std::unique_ptr<D3D12Texture> TextureManager::CreateTextureFromPath(const std::filesystem::path& texturePath) const
{
	if (!m_assetSystem || !m_rhi || !m_descriptorHeapManager)
	{
		LOG_FATAL("TextureManager::CreateTextureFromPath: manager dependencies are unavailable.");
		return nullptr;
	}

	return std::make_unique<D3D12Texture>(*m_rhi, LoadTexturePayload(*m_assetSystem, texturePath), *m_descriptorHeapManager);
}

TextureManager::TextureManager(const AssetSystem& assetSystem, D3D12Rhi& rhi, D3D12DescriptorHeapManager& descriptorHeapManager) noexcept :
    m_assetSystem(&assetSystem), m_rhi(&rhi), m_descriptorHeapManager(&descriptorHeapManager)
{
	LoadDefaults();
}

TextureManager::~TextureManager() noexcept
{
	UnloadAll();
}

void TextureManager::LoadDefaults()
{
	LoadTexture(TextureId::Checker, "ColorCheckerBoard.png");
	LoadTexture(TextureId::SkyCubemap, "SkyCubemap.png");
	LoadDefaultTextures();

	LOG_INFO(std::format("TextureManager: Loaded {} default textures", GetLoadedCount()));
}

void TextureManager::LoadTexture(TextureId id, const std::filesystem::path& relativePath)
{
	const auto index = static_cast<std::size_t>(id);
	if (index >= kTextureCount)
	{
		LOG_ERROR(std::format("TextureManager::LoadTexture: Invalid texture ID {}", index));
		return;
	}

	if (m_textures[index])
	{
		LOG_DEBUG(std::format("TextureManager: Replacing texture at slot {}", index));
		m_textures[index].reset();
	}

	m_textures[index] = CreateTextureFromPath(relativePath);
	if (!m_textures[index])
	{
		LOG_ERROR(std::format("TextureManager::LoadTexture: Failed to load '{}' into slot {}", relativePath.string(), index));
		return;
	}

	LOG_DEBUG(std::format("TextureManager: Loaded '{}' at slot {}", relativePath.string(), index));
}

D3D12Texture* TextureManager::LoadFromPath(const std::filesystem::path& texturePath)
{
	const std::filesystem::path resolvedPath = ResolveTexturePath(texturePath);
	if (resolvedPath.empty())
	{
		LOG_WARNING(std::format("TextureManager::LoadFromPath: Failed to resolve '{}'", texturePath.string()));
		return nullptr;
	}

	const TextureCacheKey cacheKey = MakeCacheKey(resolvedPath);
	if (cacheKey.empty())
	{
		LOG_WARNING(std::format("TextureManager::LoadFromPath: Failed to canonicalize '{}'", resolvedPath.string()));
		return nullptr;
	}

	if (auto it = m_pathTextures.find(cacheKey); it != m_pathTextures.end())
	{
		return it->second.get();
	}

	auto texture = CreateTextureFromPath(resolvedPath);
	if (!texture)
	{
		LOG_WARNING(std::format("TextureManager::LoadFromPath: Failed to create texture for '{}'", resolvedPath.string()));
		return nullptr;
	}

	D3D12Texture* texturePtr = texture.get();
	m_pathTextures.emplace(cacheKey, std::move(texture));

	LOG_DEBUG(std::format("TextureManager: Cached '{}'", resolvedPath.string()));
	return texturePtr;
}

void TextureManager::UnloadTexture(TextureId id) noexcept
{
	const auto index = static_cast<std::size_t>(id);
	if (index < kTextureCount)
	{
		m_textures[index].reset();
	}
}

void TextureManager::UnloadAll() noexcept
{
	m_pathTextures.clear();
	for (auto& texture : m_textures)
	{
		texture.reset();
	}
}

D3D12Texture* TextureManager::GetTexture(TextureId id) noexcept
{
	return const_cast<D3D12Texture*>(std::as_const(*this).GetTexture(id));
}

const D3D12Texture* TextureManager::GetTexture(TextureId id) const noexcept
{
	const auto index = static_cast<std::size_t>(id);
	return (index < kTextureCount) ? m_textures[index].get() : nullptr;
}

D3D12Texture* TextureManager::GetDefaultTexture(DefaultTexture type)
{
	if (D3D12Texture* texture = LoadFromPath(DefaultTextures::GetPath(type)))
	{
		return texture;
	}

	LOG_WARNING(std::format("TextureManager: Falling back to checker for {} default texture", DefaultTextures::GetName(type)));
	return GetTexture(TextureId::Checker);
}

const D3D12Texture* TextureManager::GetDefaultTexture(DefaultTexture type) const
{
	if (const D3D12Texture* texture = FindPathTexture(DefaultTextures::GetPath(type)))
	{
		return texture;
	}

	return GetTexture(TextureId::Checker);
}

bool TextureManager::IsLoaded(TextureId id) const noexcept
{
	const auto index = static_cast<std::size_t>(id);
	return (index < kTextureCount) && (m_textures[index] != nullptr);
}

std::size_t TextureManager::GetLoadedCount() const noexcept
{
	std::size_t count = 0;
	for (const auto& texture : m_textures)
	{
		if (texture)
			++count;
	}
	return count + m_pathTextures.size();
}

void TextureManager::LoadDefaultTextures()
{
	for (std::uint8_t index = 0; index < static_cast<std::uint8_t>(DefaultTexture::Count); ++index)
	{
		const auto type = static_cast<DefaultTexture>(index);
		if (!LoadFromPath(DefaultTextures::GetPath(type)))
		{
			LOG_WARNING(
			    std::format(
			        "TextureManager: Could not preload {} default texture; checker remains the emergency fallback",
			        DefaultTextures::GetName(type)));
		}
	}
}

const D3D12Texture* TextureManager::FindPathTexture(const std::filesystem::path& texturePath) const noexcept
{
	const std::filesystem::path resolvedPath = ResolveTexturePath(texturePath);
	if (resolvedPath.empty())
	{
		return nullptr;
	}

	const TextureCacheKey cacheKey = MakeCacheKey(resolvedPath);
	if (cacheKey.empty())
	{
		return nullptr;
	}

	if (auto it = m_pathTextures.find(cacheKey); it != m_pathTextures.end())
	{
		return it->second.get();
	}

	return nullptr;
}

std::filesystem::path TextureManager::ResolveTexturePath(const std::filesystem::path& texturePath) const
{
	if (!m_assetSystem)
	{
		return {};
	}

	if (auto resolved = m_assetSystem->ResolvePath(texturePath, AssetType::Texture))
	{
		return Engine::FileSystem::NormalizePath(*resolved);
	}

	return {};
}

TextureManager::TextureCacheKey TextureManager::MakeCacheKey(const std::filesystem::path& resolvedPath) const
{
	if (resolvedPath.empty())
	{
		return {};
	}

	std::wstring key = resolvedPath.generic_wstring();
	std::transform(
	    key.begin(),
	    key.end(),
	    key.begin(),
	    [](wchar_t value)
	    {
		    return static_cast<wchar_t>(std::towlower(value));
	    });
	return key;
}
