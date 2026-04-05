#include "PCH.h"

#include "Renderer/Public/Textures/TextureManager.h"

#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Paths/PathUtils.h"
#include "D3D12/Textures/TextureFactory.h"
#include "D3D12/Textures/TextureLoader.h"
#include "Resources/Texture.h"

#include <format>

std::unique_ptr<Texture> TextureManager::CreateTextureFromPath(const std::filesystem::path& texturePath) const
{
	if (!m_textureFactory)
	{
		LOG_FATAL("TextureManager::CreateTextureFromPath: texture factory is unavailable.");
		return nullptr;
	}

	return m_textureFactory->CreateTexture(TextureLoader::Load(texturePath));
}

TextureManager::TextureManager(D3D12Rhi& rhi, D3D12DescriptorHeapManager& descriptorHeapManager) noexcept :
	 m_textureFactory(TextureFactory::Create(rhi, descriptorHeapManager))
{
	LoadDefaults();
}

TextureManager::~TextureManager() noexcept
{
	UnloadAll();
}

void TextureManager::LoadDefaults()
{
	LoadTexture(TextureId::Checker, DefaultTextures::GetPath(DefaultTexture::Checkerboard));
	LoadTexture(TextureId::SkyCubemap, DefaultTextures::GetPath(DefaultTexture::Cubemap));
	LoadDefaultTextures();

	LOG_INFO(std::format("TextureManager: Loaded {} default textures", GetLoadedCount()));
}

void TextureManager::LoadSceneTextures(const TextureSnapshot& textureSnapshot)
{
	for (const std::filesystem::path& texturePath : textureSnapshot.texturePaths)
	{
		LoadFromPath(texturePath);
	}
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

Texture* TextureManager::LoadFromPath(const std::filesystem::path& texturePath)
{
	const auto resolvedPathResult = Filesystem::ResolveAssetPathNormalized(texturePath, AssetType::Texture);
	if (!resolvedPathResult)
	{
		LOG_WARNING(std::format("TextureManager::LoadFromPath: Failed to resolve '{}'", texturePath.string()));
		return nullptr;
	}

	const std::filesystem::path& resolvedPath = *resolvedPathResult;

	const TextureCacheKey cacheKey = Engine::Paths::MakePathKey(resolvedPath);
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

	Texture* texturePtr = texture.get();
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

void TextureManager::UnloadSceneTextures() noexcept
{
	for (auto it = m_pathTextures.begin(); it != m_pathTextures.end();)
	{
		if (m_defaultPathTextureKeys.contains(it->first))
		{
			++it;
			continue;
		}

		it = m_pathTextures.erase(it);
	}
}

void TextureManager::UnloadAll() noexcept
{
	m_pathTextures.clear();
	m_defaultPathTextureKeys.clear();
	for (auto& texture : m_textures)
	{
		texture.reset();
	}
}

Texture* TextureManager::GetTexture(TextureId id) noexcept
{
	return const_cast<Texture*>(std::as_const(*this).GetTexture(id));
}

const Texture* TextureManager::GetTexture(TextureId id) const noexcept
{
	const auto index = static_cast<std::size_t>(id);
	return (index < kTextureCount) ? m_textures[index].get() : nullptr;
}

Texture* TextureManager::GetSceneTexture(const std::filesystem::path& texturePath) noexcept
{
	return const_cast<Texture*>(std::as_const(*this).GetSceneTexture(texturePath));
}

const Texture* TextureManager::GetSceneTexture(const std::filesystem::path& texturePath) const noexcept
{
	return FindPathTexture(texturePath);
}

const Texture* TextureManager::ResolveTextureOrDefault(
	const std::optional<std::filesystem::path>& texturePath,
	DefaultTexture fallbackType) const
{
	if (texturePath)
	{
		if (const Texture* texture = GetSceneTexture(*texturePath))
		{
			return texture;
		}
	}

	if (const Texture* texture = FindPathTexture(DefaultTextures::GetPath(fallbackType)))
	{
		return texture;
	}

	LOG_WARNING(std::format("TextureManager: Falling back to checkerboard for {} default texture", DefaultTextures::GetName(fallbackType)));
	if (const Texture* texture = FindPathTexture(DefaultTextures::GetPath(DefaultTexture::Checkerboard)))
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
		{
			++count;
		}
	}

	return count + m_pathTextures.size();
}

void TextureManager::LoadDefaultTextures()
{
	for (std::uint8_t index = 0; index < static_cast<std::uint8_t>(DefaultTexture::Count); ++index)
	{
		const auto type = static_cast<DefaultTexture>(index);
		RegisterDefaultPathTexture(DefaultTextures::GetPath(type));
		if (!LoadFromPath(DefaultTextures::GetPath(type)))
		{
			LOG_WARNING(
				std::format(
					"TextureManager: Could not preload {} default texture; checker remains the emergency fallback",
					DefaultTextures::GetName(type)));
		}
	}
}

void TextureManager::RegisterDefaultPathTexture(const std::filesystem::path& texturePath)
{
	const auto resolvedPath = Filesystem::ResolveAssetPathNormalized(texturePath, AssetType::Texture);
	if (!resolvedPath)
	{
		return;
	}

	const TextureCacheKey cacheKey = Engine::Paths::MakePathKey(*resolvedPath);
	if (cacheKey.empty())
	{
		return;
	}

	m_defaultPathTextureKeys.insert(cacheKey);
}

const Texture* TextureManager::FindPathTexture(const std::filesystem::path& texturePath) const noexcept
{
	const auto resolvedPath = Filesystem::ResolveAssetPathNormalized(texturePath, AssetType::Texture);
	if (!resolvedPath)
	{
		return nullptr;
	}

	const TextureCacheKey cacheKey = Engine::Paths::MakePathKey(*resolvedPath);
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
