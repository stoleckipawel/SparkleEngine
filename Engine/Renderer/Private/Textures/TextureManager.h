#pragma once

#include "Renderer/Public/Resources/Textures/DefaultTextures.h"
#include "Renderer/Public/Resources/Textures/TextureDiagnostics.h"
#include "RHI/Public/Resources/RhiResourceHandles.h"
#include "Rendering/RenderInputFrame.h"
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

class TextureManager final
{
  public:
	using PreviewTextureResolver = TexturePreviewHandleResolver;

	TextureManager(
	    RhiResourceService& resourceService,
	    RhiDescriptorService& descriptorService,
	    RhiUploadService& uploadService) noexcept;

	~TextureManager() noexcept;

	TextureManager(const TextureManager&) = delete;
	TextureManager& operator=(const TextureManager&) = delete;
	TextureManager(TextureManager&&) = delete;
	TextureManager& operator=(TextureManager&&) = delete;

	std::vector<RhiResourceHandle> LoadSceneTextures(
	    const RenderTextureTable& textures,
	    RenderCommandList& commandList);
	bool HasPendingSceneTextureUploads(const RenderTextureTable& textures) const noexcept;

	void UnloadSceneTextures() noexcept;
	void UnloadAll() noexcept;

	const RendererTexture* GetTexture(TextureId id) const noexcept;
	const RendererTexture* ResolveDefaultSkyTexture() const noexcept;
	const RendererTexture* GetSceneTexture(const std::filesystem::path& texturePath) const noexcept;
	const RendererTexture* ResolveTextureReferenceOrDefault(
	    const Assets::CookedTextureReference* textureReference,
	    DefaultTexture fallbackType)
	    const;

	TextureDiagnosticsSnapshot CaptureDiagnosticsSnapshot(
	    const PreviewTextureResolver& resolvePreviewTexture) const;

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

	void LoadDefaults(RenderCommandList& commandList, std::vector<RhiResourceHandle>& uploadedResources);
	void LoadTexture(
	    TextureId id,
	    const std::filesystem::path& relativePath,
	    RenderCommandList& commandList,
	    std::vector<RhiResourceHandle>& uploadedResources);
	void LoadDefaultTextures(RenderCommandList& commandList, std::vector<RhiResourceHandle>& uploadedResources);
	RendererTexture* LoadFromPath(
	    const std::filesystem::path& texturePath,
	    RenderCommandList& commandList,
	    std::vector<RhiResourceHandle>& uploadedResources);
	std::optional<RendererTexture> CreateTextureFromPath(
	    const std::filesystem::path& texturePath,
	    RenderCommandList& commandList,
	    std::vector<RhiResourceHandle>& uploadedResources) const;
	const RendererTexture* FindPathTexture(const std::filesystem::path& texturePath) const noexcept;
	std::optional<ResolvedTexturePath> ResolveTexturePath(const std::filesystem::path& texturePath) const noexcept;
	void ReleaseTexture(RendererTexture& texture) noexcept;
	void RegisterDefaultPathTexture(const std::filesystem::path& texturePath);
	TextureDiagnosticsRow BuildDiagnosticsRow(
	    const RendererTexture& texture,
	    TextureDiagnosticsKind kind,
	    const std::string& key,
	    const PreviewTextureResolver& resolvePreviewTexture) const;
};
