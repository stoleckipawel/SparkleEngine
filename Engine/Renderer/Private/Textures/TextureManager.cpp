#include "PCH.h"

#include "Textures/TextureManager.h"

#include "Assets/Cooked/CookedTextureReference.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Paths/PathUtils.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Resources/Texture.h"

#include <algorithm>
#include <format>

static const auto g_textureManagerLogger = Logging::GetOrCreateLogger("Renderer.TextureManager");

std::unique_ptr<Texture> TextureManager::CreateTextureFromPath(const std::filesystem::path& texturePath) const
{
	if (m_renderHardwareInterface == nullptr)
	{
		Diagnostics::Fail(
		    g_textureManagerLogger,
		    __FILE__,
		    __LINE__,
		    "TextureManager::CreateTextureFromPath: render hardware interface is unavailable.");
		return nullptr;
	}

	return m_renderHardwareInterface->CreateTextureFromPath(texturePath);
}

TextureManager::TextureManager(RenderHardwareInterface& renderHardwareInterface) noexcept :
    m_renderHardwareInterface(&renderHardwareInterface)
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

	SPDLOG_LOGGER_INFO(g_textureManagerLogger, "{}", std::format("TextureManager: Loaded {} default textures", GetLoadedCount()));
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
		SPDLOG_LOGGER_ERROR(g_textureManagerLogger, "{}", std::format("TextureManager::LoadTexture: Invalid texture ID {}", index));
		return;
	}

	if (m_textures[index])
	{
		SPDLOG_LOGGER_DEBUG(g_textureManagerLogger, "{}", std::format("TextureManager: Replacing texture at slot {}", index));
		m_textures[index].reset();
	}

	m_textures[index] = CreateTextureFromPath(relativePath);
	if (!m_textures[index])
	{
		SPDLOG_LOGGER_ERROR(
		    g_textureManagerLogger,
		    "{}",
		    std::format("TextureManager::LoadTexture: Failed to load '{}' into slot {}", relativePath.string(), index));
		return;
	}

	SPDLOG_LOGGER_DEBUG(g_textureManagerLogger, "{}", std::format("TextureManager: Loaded '{}' at slot {}", relativePath.string(), index));
}

Texture* TextureManager::LoadFromPath(const std::filesystem::path& texturePath)
{
	const auto resolvedPathResult = Filesystem::ResolveAssetPathNormalized(texturePath, AssetType::Texture);
	if (!resolvedPathResult)
	{
		SPDLOG_LOGGER_WARN(
		    g_textureManagerLogger,
		    "{}",
		    std::format("TextureManager::LoadFromPath: Failed to resolve '{}'", texturePath.string()));
		return nullptr;
	}

	const std::filesystem::path& resolvedPath = *resolvedPathResult;

	const TextureCacheKey cacheKey = Paths::MakePathKey(resolvedPath);
	if (cacheKey.empty())
	{
		SPDLOG_LOGGER_WARN(
		    g_textureManagerLogger,
		    "{}",
		    std::format("TextureManager::LoadFromPath: Failed to canonicalize '{}'", resolvedPath.string()));
		return nullptr;
	}

	if (auto it = m_pathTextures.find(cacheKey); it != m_pathTextures.end())
	{
		return it->second.get();
	}

	auto texture = CreateTextureFromPath(resolvedPath);
	if (!texture)
	{
		SPDLOG_LOGGER_WARN(
		    g_textureManagerLogger,
		    "{}",
		    std::format("TextureManager::LoadFromPath: Failed to create texture for '{}'", resolvedPath.string()));
		return nullptr;
	}

	Texture* texturePtr = texture.get();
	m_pathTextures.emplace(cacheKey, std::move(texture));

	SPDLOG_LOGGER_DEBUG(g_textureManagerLogger, "{}", std::format("TextureManager: Cached '{}'", resolvedPath.string()));
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

const Texture* TextureManager::ResolveTextureReferenceOrDefault(
    const Assets::CookedTextureReference* textureReference,
    DefaultTexture fallbackType) const
{
	if (textureReference && textureReference->IsValid())
	{
		if (const Texture* texture = GetSceneTexture(textureReference->texturePath))
		{
			return texture;
		}
	}

	if (const Texture* texture = FindPathTexture(DefaultTextures::GetPath(fallbackType)))
	{
		return texture;
	}

	SPDLOG_LOGGER_WARN(
	    g_textureManagerLogger,
	    "{}",
	    std::format("TextureManager: Falling back to checkerboard for {} default texture", DefaultTextures::GetName(fallbackType)));
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

TextureDiagnosticsSnapshot TextureManager::CaptureDiagnosticsSnapshot() const
{
	TextureDiagnosticsSnapshot snapshot;
	snapshot.Rows.reserve(kTextureCount + m_pathTextures.size());

	for (std::size_t index = 0; index < kTextureCount; ++index)
	{
		const std::unique_ptr<Texture>& texture = m_textures[index];
		if (!texture)
		{
			continue;
		}

		snapshot.Rows.push_back(BuildDiagnosticsRow(
		    *texture,
		    TextureDiagnosticsKind::DefaultSlot,
		    std::format("slot:{}", index)));
	}

	for (const auto& [cacheKey, texture] : m_pathTextures)
	{
		if (!texture)
		{
			continue;
		}

		const bool defaultOrFallback = m_defaultPathTextureKeys.contains(cacheKey);
		const TextureDiagnosticsKind kind = defaultOrFallback ? TextureDiagnosticsKind::DefaultPath : TextureDiagnosticsKind::Scene;
		snapshot.Rows.push_back(BuildDiagnosticsRow(
		    *texture,
		    kind,
		    std::filesystem::path(cacheKey).generic_string()));
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

void TextureManager::LoadDefaultTextures()
{
	for (std::uint8_t index = 0; index < static_cast<std::uint8_t>(DefaultTexture::Count); ++index)
	{
		const auto type = static_cast<DefaultTexture>(index);
		RegisterDefaultPathTexture(DefaultTextures::GetPath(type));
		if (!LoadFromPath(DefaultTextures::GetPath(type)))
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
	const auto resolvedPath = Filesystem::ResolveAssetPathNormalized(texturePath, AssetType::Texture);
	if (!resolvedPath)
	{
		return;
	}

	const TextureCacheKey cacheKey = Paths::MakePathKey(*resolvedPath);
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

	const TextureCacheKey cacheKey = Paths::MakePathKey(*resolvedPath);
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

TextureDiagnosticsRow TextureManager::BuildDiagnosticsRow(
    const Texture& texture,
    TextureDiagnosticsKind kind,
	const std::string& key)
{
	const TextureRuntimeInfo textureInfo = texture.GetRuntimeInfo();
	TextureDiagnosticsRow row;
	row.Key = key;
	row.Kind = kind;
	row.Dimension = textureInfo.Dimension;
	row.FormatIntent = textureInfo.FormatIntent;
	row.ResidencyState = textureInfo.IsValid ? TextureDiagnosticsResidencyState::Resident : TextureDiagnosticsResidencyState::Unloaded;
	row.Width = textureInfo.Width;
	row.Height = textureInfo.Height;
	row.ArraySize = textureInfo.ArraySize;
	row.Format = textureInfo.FormatName;
	row.MipCount = textureInfo.MipCount;
	row.EstimatedByteSize = textureInfo.EstimatedByteSize;
	row.GpuShaderResourceViewId = textureInfo.GpuShaderResourceViewId;
	row.Loaded = textureInfo.IsValid;
	row.StreamManaged = false;
	return row;
}
