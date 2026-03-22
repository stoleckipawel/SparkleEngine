#pragma once

#include "Renderer/Public/Textures/DefaultTextures.h"
#include "Renderer/Public/RendererAPI.h"
#include "GameFramework/Public/Scene/Textures/TextureSnapshot.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <unordered_set>
#include <unordered_map>
#include <string>

class D3D12DescriptorHeapManager;
class D3D12Rhi;
class D3D12Texture;

enum class TextureId : uint8_t
{
	Checker,
	SkyCubemap,

	Count
};

class SPARKLE_RENDERER_API TextureManager final
{
  public:
	TextureManager(D3D12Rhi& rhi, D3D12DescriptorHeapManager& descriptorHeapManager) noexcept;

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

	D3D12Texture* GetTexture(TextureId id) noexcept;
	const D3D12Texture* GetTexture(TextureId id) const noexcept;
	D3D12Texture* GetSceneTexture(const std::filesystem::path& texturePath) noexcept;
	const D3D12Texture* GetSceneTexture(const std::filesystem::path& texturePath) const noexcept;
	const D3D12Texture* ResolveTextureOrDefault(
	    const std::optional<std::filesystem::path>& texturePath,
	    DefaultTexture fallbackType) const;

	bool IsLoaded(TextureId id) const noexcept;

	std::size_t GetLoadedCount() const noexcept;

  private:
	D3D12Rhi* m_rhi = nullptr;
	D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;

	static constexpr std::size_t kTextureCount = static_cast<std::size_t>(TextureId::Count);
	using TextureCacheKey = std::wstring;
	std::array<std::unique_ptr<D3D12Texture>, kTextureCount> m_textures{};
	std::unordered_map<TextureCacheKey, std::unique_ptr<D3D12Texture>> m_pathTextures;
	std::unordered_set<TextureCacheKey> m_defaultPathTextureKeys;

	void LoadDefaultTextures();
	D3D12Texture* LoadFromPath(const std::filesystem::path& texturePath);
	std::unique_ptr<D3D12Texture> CreateTextureFromPath(const std::filesystem::path& texturePath) const;
	const D3D12Texture* FindPathTexture(const std::filesystem::path& texturePath) const noexcept;
	std::filesystem::path ResolveTexturePath(const std::filesystem::path& texturePath) const;
	TextureCacheKey MakeCacheKey(const std::filesystem::path& resolvedPath) const;
	void RegisterDefaultPathTexture(const std::filesystem::path& texturePath);
};
