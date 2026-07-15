#pragma once

#include "Renderer/Public/Resources/Textures/DefaultTextures.h"
#include "Renderer/Public/Resources/Textures/TextureDiagnostics.h"
#include "Renderer/Public/RendererAPI.h"
#include "RHI/Public/Interop/RhiNativeHandles.h"
#include "Scene/Textures/TextureSnapshot.h"
#include "Textures/RendererTexture.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <vector>

class RenderCommandList;
class RhiDescriptorService;
class RhiResourceService;
class RhiUploadService;
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
	TextureManager(
	    RhiResourceService& resourceService,
	    RhiDescriptorService& descriptorService,
	    RhiUploadService& uploadService) noexcept;

	~TextureManager() noexcept;

	TextureManager(const TextureManager&) = delete;
	TextureManager& operator=(const TextureManager&) = delete;
	TextureManager(TextureManager&&) = delete;
	TextureManager& operator=(TextureManager&&) = delete;

	std::vector<NativeResourceHandle> LoadSceneTextures(
	    const TextureSnapshot& textureSnapshot,
	    RenderCommandList& commandList);
	bool HasPendingSceneTextureUploads(const TextureSnapshot& textureSnapshot) const noexcept;

	void UnloadSceneTextures() noexcept;
	void UnloadAll() noexcept;

	const RendererTexture* GetTexture(TextureId id) const noexcept;
	const RendererTexture* ResolveDefaultSkyTexture() const noexcept;
	const RendererTexture* GetSceneTexture(const std::filesystem::path& texturePath) const noexcept;
	const RendererTexture* ResolveTextureReferenceOrDefault(
	    const Assets::CookedTextureReference* textureReference,
	    DefaultTexture fallbackType)
	    const;

	TextureDiagnosticsSnapshot CaptureDiagnosticsSnapshot() const;

  private:
	RhiResourceService& m_resourceService;
	RhiDescriptorService& m_descriptorService;
	RhiUploadService& m_uploadService;

	static constexpr std::size_t kTextureCount = static_cast<std::size_t>(TextureId::Count);
	using TextureCacheKey = std::wstring;
	struct ResolvedTexturePath final
	{
		std::filesystem::path Path;
		TextureCacheKey CacheKey;
	};

	std::array<std::optional<RendererTexture>, kTextureCount> m_textures{};
	std::unordered_map<TextureCacheKey, RendererTexture> m_pathTextures;
	std::unordered_set<TextureCacheKey> m_defaultPathTextureKeys;
	bool m_defaultsLoaded = false;

	void LoadDefaults(RenderCommandList& commandList, std::vector<NativeResourceHandle>& uploadedResources);
	void LoadTexture(
	    TextureId id,
	    const std::filesystem::path& relativePath,
	    RenderCommandList& commandList,
	    std::vector<NativeResourceHandle>& uploadedResources);
	void LoadDefaultTextures(RenderCommandList& commandList, std::vector<NativeResourceHandle>& uploadedResources);
	RendererTexture* LoadFromPath(
	    const std::filesystem::path& texturePath,
	    RenderCommandList& commandList,
	    std::vector<NativeResourceHandle>& uploadedResources);
	std::optional<RendererTexture> CreateTextureFromPath(
	    const std::filesystem::path& texturePath,
	    RenderCommandList& commandList,
	    std::vector<NativeResourceHandle>& uploadedResources) const;
	const RendererTexture* FindPathTexture(const std::filesystem::path& texturePath) const noexcept;
	std::optional<ResolvedTexturePath> ResolveTexturePath(const std::filesystem::path& texturePath) const noexcept;
	void ReleaseTexture(RendererTexture& texture) noexcept;
	void RegisterDefaultPathTexture(const std::filesystem::path& texturePath);
	TextureDiagnosticsRow BuildDiagnosticsRow(const RendererTexture& texture, TextureDiagnosticsKind kind, const std::string& key) const;
};
