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

class TextureCache final
{
  public:
	using PreviewTextureResolver = TexturePreviewHandleResolver;

	TextureCache(
	    RhiResourceService& resourceService,
	    RhiDescriptorService& descriptorService,
	    RhiUploadService& uploadService,
	    RhiCommandSubmissionService& submissions,
	    TaskExecutor& taskExecutor,
	    TaskScope& applicationScope);
	~TextureCache() noexcept;

	TextureCache(const TextureCache&) = delete;
	TextureCache& operator=(const TextureCache&) = delete;
	TextureCache(TextureCache&&) = delete;
	TextureCache& operator=(TextureCache&&) = delete;

	std::vector<RhiResourceHandle> LoadSceneTextures(const RenderTextureTable& textures, RenderCommandList& commandList);
	bool HasPendingSceneTextureUploads() const noexcept;
	void RecordUploadSubmission(RhiSubmissionToken token) noexcept;
	void PollResidency() noexcept;
	void CommitBindingRevision(std::uint64_t bindingRevision) noexcept;

	void UnloadSceneTextures() noexcept;
	void UnloadAll() noexcept;

	const RendererTexture* GetTexture(TextureId id) const noexcept;
	const RendererTexture* ResolveDefaultSkyTexture() const noexcept;
	const RendererTexture* GetSceneTexture(const std::filesystem::path& texturePath) const noexcept;
	const RendererTexture* ResolveTextureReferenceOrSemanticDefault(
	    const Assets::CookedTextureReference* textureReference,
	    DefaultTexture defaultType) const;
	std::uint64_t GetBindingRevision() const noexcept { return m_bindingRevision; }

	TextureDiagnosticsSnapshot CaptureDiagnosticsSnapshot(const PreviewTextureResolver& resolvePreviewTexture) const;

  private:
	using TextureKey = std::wstring;

	struct ResolvedTexturePath final
	{
		std::filesystem::path Path;
		TextureKey CacheKey;
	};

	struct TextureLoadPayload final
	{
		CookedTextureFilePayload File;
		LoadedTextureData Texture;
	};

	struct TextureRequest final
	{
		ResolvedTexturePath Source;
		AssetGenerationHandle Generation;
		TaskExecution Execution;
		std::shared_ptr<TextureLoadPayload> Payload;
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

	void LoadDefaultTextures(RenderCommandList& commandList, std::vector<RhiResourceHandle>& uploadedResources);
	void RequestTexture(const ResolvedTexturePath& source, std::uint32_t generation);
	void SynchronizeSceneTextures(const RenderTextureTable& textures);
	void ConsumeCompletedRequests() noexcept;
	void UploadReadyTextures(RenderCommandList& commandList, std::vector<RhiResourceHandle>& uploadedResources);
	RendererTexture CreateTexture(
	    const std::filesystem::path& texturePath,
	    LoadedTextureData& loadedTexture,
	    RenderCommandList& commandList) const;
	TextureRequest* FindRequest(const TextureKey& cacheKey, std::uint32_t generation) noexcept;
	const TextureRequest* FindRequest(const TextureKey& cacheKey, std::uint32_t generation) const noexcept;
	void ActivateResidentRequests() noexcept;
	void QueueTextureRetirement(ActiveTexture&& texture, std::uint64_t bindingRevision);
	void ReleaseActiveTexture(ActiveTexture& texture) noexcept;
	RhiSubmissionState CaptureLastSubmittedState() const noexcept;
	const RendererTexture* FindPathTexture(const std::filesystem::path& texturePath) const noexcept;
	std::optional<ResolvedTexturePath> ResolveTexturePath(const std::filesystem::path& texturePath) const noexcept;
	void ReleaseTexture(RendererTexture& texture) noexcept;
	TextureDiagnosticsRow BuildDiagnosticsRow(
	    const RendererTexture& texture,
	    TextureDiagnosticsKind kind,
	    const std::string& key,
	    const PreviewTextureResolver& resolvePreviewTexture) const;
	static std::uint64_t CalculateTexturePayloadBytes(const RhiTextureUploadDesc& textureUpload) noexcept;
	static std::uint64_t MakeAssetKey(const TextureKey& cacheKey);

	RhiResourceService& m_resourceService;
	RhiDescriptorService& m_descriptorService;
	RhiUploadService& m_uploadService;
	RhiCommandSubmissionService& m_submissions;
	TaskExecutor& m_taskExecutor;
	std::unique_ptr<TaskScope> m_taskScope;
	AssetResidency m_residency;
	std::array<TextureKey, kTextureCount> m_textureSlotKeys;
	std::unordered_map<TextureKey, RendererTexture> m_defaultTextures;
	std::unordered_map<TextureKey, ActiveTexture> m_pathTextures;
	std::vector<RetiredTexture> m_retiredTextures;
	std::unordered_set<TextureKey> m_wantedPathTextureKeys;
	std::vector<TextureRequest> m_requests;
	std::uint32_t m_sceneTextureGeneration = 0;
	std::uint64_t m_bindingRevision = 0;
	bool m_defaultsLoaded = false;
};
