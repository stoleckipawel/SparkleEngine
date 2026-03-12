// ============================================================================
// TextureManager.h
// ----------------------------------------------------------------------------
// Centralized texture resource management with caching and lifecycle control.
//
#pragma once

#include "Renderer/Public/Textures/DefaultTextures.h"
#include "Renderer/Public/RendererAPI.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <string>

class AssetSystem;
class D3D12DescriptorHeapManager;
class D3D12Rhi;
class D3D12Texture;

// ============================================================================
// TextureId Enumeration
// ============================================================================

/// Well-known texture identifiers for fast, type-safe lookups.
/// Add new entries before Count; do not reorder existing entries.
enum class TextureId : uint8_t
{
	Checker,     ///< Default checker pattern for debugging
	SkyCubemap,  ///< Environment cubemap for skybox/reflections

	Count  ///< Number of texture slots (must be last)
};

// ============================================================================
// TextureManager
// ============================================================================

class SPARKLE_RENDERER_API TextureManager final
{
  public:
	// ========================================================================
	// Lifecycle
	// ========================================================================

	TextureManager(const AssetSystem& assetSystem, D3D12Rhi& rhi, D3D12DescriptorHeapManager& descriptorHeapManager) noexcept;

	~TextureManager() noexcept;

	TextureManager(const TextureManager&) = delete;
	TextureManager& operator=(const TextureManager&) = delete;
	TextureManager(TextureManager&&) = delete;
	TextureManager& operator=(TextureManager&&) = delete;

	// ========================================================================
	// Loading
	// ========================================================================

	/// Loads all default engine textures (checker, cubemap, etc.).
	void LoadDefaults();

	/// Loads a texture from file and stores it at the given ID slot.
	/// Replaces any existing texture at that slot.
	/// @param id Texture slot identifier
	/// @param relativePath Path relative to textures asset directory
	void LoadTexture(TextureId id, const std::filesystem::path& relativePath);

	/// Loads or reuses a texture from the material path cache.
	/// Paths are resolved through AssetSystem and cached by canonical absolute path.
	D3D12Texture* LoadFromPath(const std::filesystem::path& texturePath);

	/// Unloads a specific texture, freeing GPU resources.
	void UnloadTexture(TextureId id) noexcept;

	/// Unloads all textures.
	void UnloadAll() noexcept;

	// ========================================================================
	// Accessors
	// ========================================================================

	D3D12Texture* GetTexture(TextureId id) noexcept;
	const D3D12Texture* GetTexture(TextureId id) const noexcept;
	D3D12Texture* GetDefaultTexture(DefaultTexture type);
	const D3D12Texture* GetDefaultTexture(DefaultTexture type) const;
	D3D12Texture* GetDefaultWhiteTexture() { return GetDefaultTexture(DefaultTexture::White); }
	const D3D12Texture* GetDefaultWhiteTexture() const { return GetDefaultTexture(DefaultTexture::White); }
	D3D12Texture* GetDefaultBlackTexture() { return GetDefaultTexture(DefaultTexture::Black); }
	const D3D12Texture* GetDefaultBlackTexture() const { return GetDefaultTexture(DefaultTexture::Black); }
	D3D12Texture* GetDefaultFlatNormalTexture() { return GetDefaultTexture(DefaultTexture::FlatNormal); }
	const D3D12Texture* GetDefaultFlatNormalTexture() const { return GetDefaultTexture(DefaultTexture::FlatNormal); }
	D3D12Texture* GetDefaultMetallicRoughnessTexture() { return GetDefaultTexture(DefaultTexture::DefaultMetallicRoughness); }
	const D3D12Texture* GetDefaultMetallicRoughnessTexture() const { return GetDefaultTexture(DefaultTexture::DefaultMetallicRoughness); }

	bool IsLoaded(TextureId id) const noexcept;

	std::size_t GetLoadedCount() const noexcept;

  private:
	// ------------------------------------------------------------------------
	// Dependencies (not owned)
	// ------------------------------------------------------------------------

	const AssetSystem* m_assetSystem = nullptr;
	D3D12Rhi* m_rhi = nullptr;
	D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;

	// ------------------------------------------------------------------------
	// Texture Storage
	// ------------------------------------------------------------------------

	static constexpr std::size_t kTextureCount = static_cast<std::size_t>(TextureId::Count);
	using TextureCacheKey = std::wstring;
	std::array<std::unique_ptr<D3D12Texture>, kTextureCount> m_textures{};
	std::unordered_map<TextureCacheKey, std::unique_ptr<D3D12Texture>> m_pathTextures;

	void LoadDefaultTextures();
	std::unique_ptr<D3D12Texture> CreateTextureFromPath(const std::filesystem::path& texturePath) const;
	const D3D12Texture* FindPathTexture(const std::filesystem::path& texturePath) const noexcept;
	std::filesystem::path ResolveTexturePath(const std::filesystem::path& texturePath) const;
	TextureCacheKey MakeCacheKey(const std::filesystem::path& resolvedPath) const;
};
