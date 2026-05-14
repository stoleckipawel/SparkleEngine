#pragma once

#include "Renderer/Public/Textures/DefaultTextures.h"
#include "Renderer/Public/Textures/TextureDiagnostics.h"
#include "Renderer/Public/RendererAPI.h"
#include "Scene/Textures/TextureSnapshot.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <string>

class RenderHardwareInterface;
class Texture;

namespace Assets
{
	struct CookedTextureReference;
}

enum class TextureId : uint8_t
{
	Checker,
	SkyCubemap,

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

	Texture* GetTexture(TextureId id) noexcept;
	const Texture* GetTexture(TextureId id) const noexcept;
	Texture* GetSceneTexture(const std::filesystem::path& texturePath) noexcept;
	const Texture* GetSceneTexture(const std::filesystem::path& texturePath) const noexcept;
	const Texture* ResolveTextureReferenceOrDefault(
	    const Assets::CookedTextureReference* textureReference,
	    DefaultTexture fallbackType) const;

	bool IsLoaded(TextureId id) const noexcept;

	std::size_t GetLoadedCount() const noexcept;
	TextureDiagnosticsSnapshot CaptureDiagnosticsSnapshot() const;

  private:
	RenderHardwareInterface* m_renderHardwareInterface = nullptr;

	static constexpr std::size_t kTextureCount = static_cast<std::size_t>(TextureId::Count);
	using TextureCacheKey = std::wstring;

	std::array<std::unique_ptr<Texture>, kTextureCount> m_textures{};
	std::unordered_map<TextureCacheKey, std::unique_ptr<Texture>> m_pathTextures;
	std::unordered_set<TextureCacheKey> m_defaultPathTextureKeys;

	void LoadDefaultTextures();
	Texture* LoadFromPath(const std::filesystem::path& texturePath);
	std::unique_ptr<Texture> CreateTextureFromPath(const std::filesystem::path& texturePath) const;
	const Texture* FindPathTexture(const std::filesystem::path& texturePath) const noexcept;
	void RegisterDefaultPathTexture(const std::filesystem::path& texturePath);
	static TextureDiagnosticsRow BuildDiagnosticsRow(
	    const Texture& texture,
	    TextureDiagnosticsKind kind,
	    const std::string& key);
};
