#pragma once

#include "Renderer/Public/Resources/Textures/DefaultTextures.h"
#include "Renderer/Public/Resources/Textures/TextureDiagnostics.h"
#include "Renderer/Public/RendererAPI.h"
#include "Scene/Textures/TextureSnapshot.h"
#include "Textures/RendererTexture.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <unordered_set>
#include <unordered_map>
#include <string>

class RenderHardwareInterface;
namespace Assets
{
	struct CookedTextureReference;
}

enum class TextureId : uint8_t
{
	Checker,
	DefaultSky,

	Count
};

class SPARKLE_RENDERER_API TextureManager final
{
  public:
	explicit TextureManager(RenderHardwareInterface& renderHardwareInterface) noexcept;

	~TextureManager() noexcept;

	TextureManager(const TextureManager&) = delete;
	TextureManager& operator=(const TextureManager&) = delete;
	TextureManager(TextureManager&&) = delete;
	TextureManager& operator=(TextureManager&&) = delete;

	void LoadDefaults();
	void LoadSceneTextures(const TextureSnapshot& textureSnapshot);

	void LoadTexture(TextureId id, const std::filesystem::path& relativePath);

	void UnloadTexture(TextureId id) noexcept;
	void UnloadSceneTextures() noexcept;

	void UnloadAll() noexcept;

	RendererTexture* GetTexture(TextureId id) noexcept;
	const RendererTexture* GetTexture(TextureId id) const noexcept;
	const RendererTexture* ResolveDefaultSkyTexture() const noexcept;
	RendererTexture* GetSceneTexture(const std::filesystem::path& texturePath) noexcept;
	const RendererTexture* GetSceneTexture(const std::filesystem::path& texturePath) const noexcept;
	const RendererTexture* ResolveTextureReferenceOrDefault(
	    const Assets::CookedTextureReference* textureReference,
	    DefaultTexture fallbackType)
	    const;

	bool IsLoaded(TextureId id) const noexcept;

	std::size_t GetLoadedCount() const noexcept;
	TextureDiagnosticsSnapshot CaptureDiagnosticsSnapshot() const;

  private:
	RenderHardwareInterface* m_renderHardwareInterface = nullptr;

	static constexpr std::size_t kTextureCount = static_cast<std::size_t>(TextureId::Count);
	using TextureCacheKey = std::wstring;

	std::array<std::optional<RendererTexture>, kTextureCount> m_textures{};
	std::unordered_map<TextureCacheKey, RendererTexture> m_pathTextures;
	std::unordered_set<TextureCacheKey> m_defaultPathTextureKeys;
	bool m_defaultsLoaded = false;

	void LoadDefaultTextures();
	RendererTexture* LoadFromPath(const std::filesystem::path& texturePath);
	std::optional<RendererTexture> CreateTextureFromPath(const std::filesystem::path& texturePath) const;
	const RendererTexture* FindPathTexture(const std::filesystem::path& texturePath) const noexcept;
	void ReleaseTexture(RendererTexture& texture) noexcept;
	void RegisterDefaultPathTexture(const std::filesystem::path& texturePath);
	TextureDiagnosticsRow BuildDiagnosticsRow(const RendererTexture& texture, TextureDiagnosticsKind kind, const std::string& key) const;
};
