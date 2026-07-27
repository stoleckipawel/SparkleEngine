#pragma once

#include "Renderer/Public/Resources/Textures/DefaultTextures.h"
#include "Renderer/Public/Resources/Textures/TextureDiagnostics.h"
#include "Rendering/RenderInputFrame.h"
#include "Resources/Residency/AssetResidency.h"
#include "RHI/Public/Resources/RhiResourceHandles.h"
#include "Tasks/Public/TaskExecution.h"
#include "Textures/CookedTextureLoader.h"
#include "Textures/RendererTexture.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class RenderCommandList;
class RhiCommandSubmissionService;
class RhiDescriptorService;
class RhiResourceService;
class RhiUploadService;
class TaskExecutor;
class TaskScope;
namespace Assets
{
	struct CookedTextureReference;
}

enum class TextureId : std::uint8_t
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
	    RhiUploadService& uploadService,
	    RhiCommandSubmissionService& submissions,
	    TaskExecutor& taskExecutor,
	    TaskScope& applicationScope);
	~TextureManager() noexcept;

	TextureManager(const TextureManager&) = delete;
	TextureManager& operator=(const TextureManager&) = delete;
	TextureManager(TextureManager&&) = delete;
	TextureManager& operator=(TextureManager&&) = delete;

	std::vector<RhiResourceHandle> LoadSceneTextures(
	    const RenderTextureTable& textures,
	    RenderCommandList& commandList);
	bool HasPendingSceneTextureUploads(const RenderTextureTable& textures) const noexcept;
	void RecordUploadSubmission(RhiSubmissionToken token) noexcept;
	void PollResidency() noexcept;
	void CommitBindingRevision(
	    std::uint64_t bindingRevision) noexcept;

	void UnloadSceneTextures() noexcept;
	void UnloadAll() noexcept;

	const RendererTexture* GetTexture(TextureId id) const noexcept;
	const RendererTexture* ResolveDefaultSkyTexture() const noexcept;
	const RendererTexture* GetSceneTexture(const std::filesystem::path& texturePath) const noexcept;
	const RendererTexture* ResolveTextureReferenceOrDefault(
	    const Assets::CookedTextureReference* textureReference,
	    DefaultTexture fallbackType) const;
	std::uint64_t GetBindingRevision() const noexcept
	{
		return m_bindingRevision;
	}

	TextureDiagnosticsSnapshot CaptureDiagnosticsSnapshot(
	    const PreviewTextureResolver& resolvePreviewTexture) const;

  private:
	using TextureCacheKey = std::wstring;

	struct ResolvedTexturePath final
	{
		std::filesystem::path Path;
		TextureCacheKey CacheKey;
	};

	struct TextureLoadResult final
	{
		CookedTextureFilePayload File;
		LoadedTextureData Texture;
		std::string ErrorMessage;
	};

	struct TextureRequest final
	{
		ResolvedTexturePath Source;
		AssetGenerationHandle Generation;
		TaskExecution Execution;
		std::shared_ptr<TextureLoadResult> Result;
		std::optional<LoadedTextureData> Decoded;
		std::optional<RendererTexture> Uploaded;
		RhiResourceHandle UploadedResource;
		bool UploadSubmitted = false;
		bool Wanted = true;
	};

	struct ActiveTexture final
	{
		AssetGenerationHandle Generation;
		RendererTexture Texture;
	};

	struct RetiredTexture final
	{
		std::uint64_t BindingRevision = 0;
		ActiveTexture Active;
	};

	static constexpr std::size_t kTextureCount = static_cast<std::size_t>(TextureId::Count);

	void RequestDefaults();
	void RequestTexture(
	    const ResolvedTexturePath& source,
	    std::uint32_t generation);
	void SynchronizeSceneTextures(
	    const RenderTextureTable& textures);
	void ConsumeCompletedRequests() noexcept;
	void UploadReadyTextures(
	    RenderCommandList& commandList,
	    std::vector<RhiResourceHandle>& uploadedResources);
	std::optional<RendererTexture> CreateTexture(
	    const std::filesystem::path& texturePath,
	    LoadedTextureData& loadedTexture,
	    RenderCommandList& commandList,
	    RhiResourceHandle& outUploadedResource) const;
	TextureRequest* FindRequest(
	    const TextureCacheKey& cacheKey,
	    std::uint32_t generation) noexcept;
	const TextureRequest* FindRequest(
	    const TextureCacheKey& cacheKey,
	    std::uint32_t generation) const noexcept;
	void ActivateResidentRequests() noexcept;
	void QueueTextureRetirement(
	    ActiveTexture&& texture,
	    std::uint64_t bindingRevision);
	void ReleaseActiveTexture(
	    ActiveTexture& texture) noexcept;
	RhiSubmissionState CaptureLastSubmittedState() const noexcept;
	const RendererTexture* FindPathTexture(const std::filesystem::path& texturePath) const noexcept;
	std::optional<ResolvedTexturePath> ResolveTexturePath(const std::filesystem::path& texturePath) const noexcept;
	void ReleaseTexture(RendererTexture& texture) noexcept;
	void RegisterDefaultPathTexture(const std::filesystem::path& texturePath);
	TextureDiagnosticsRow BuildDiagnosticsRow(
	    const RendererTexture& texture,
	    TextureDiagnosticsKind kind,
	    const std::string& key,
	    const PreviewTextureResolver& resolvePreviewTexture) const;
	static std::uint64_t CalculateTexturePayloadBytes(
	    const RhiTextureUploadDesc& textureUpload) noexcept;
	static std::uint64_t MakeAssetKey(
	    const TextureCacheKey& cacheKey) noexcept;

	RhiResourceService& m_resourceService;
	RhiDescriptorService& m_descriptorService;
	RhiUploadService& m_uploadService;
	RhiCommandSubmissionService& m_submissions;
	TaskExecutor& m_taskExecutor;
	std::unique_ptr<TaskScope> m_taskScope;
	AssetResidency m_residency;
	std::array<TextureCacheKey, kTextureCount> m_textureSlotKeys;
	std::unordered_map<TextureCacheKey, ActiveTexture> m_pathTextures;
	std::vector<RetiredTexture> m_retiredTextures;
	std::unordered_set<TextureCacheKey> m_defaultPathTextureKeys;
	std::unordered_set<TextureCacheKey> m_wantedPathTextureKeys;
	std::vector<TextureRequest> m_requests;
	std::uint32_t m_sceneTextureGeneration = 0;
	std::uint64_t m_bindingRevision = 0;
	bool m_defaultsRequested = false;
};
