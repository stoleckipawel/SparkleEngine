#pragma once

#include "Textures/RendererTexture.h"

#include <cstdint>
#include <filesystem>

class RenderCommandList;
class RhiDescriptorService;
class RhiResourceService;
class RhiUploadService;
struct LoadedTextureData;
struct RhiTextureUploadDesc;

class RendererTextureFactory final
{
public:
	RendererTextureFactory(
	    RhiResourceService& resourceService,
	    RhiDescriptorService& descriptorService,
	    RhiUploadService& uploadService) noexcept;

	RendererTexture Create(
	    const std::filesystem::path& texturePath,
	    LoadedTextureData& loadedTexture,
	    RenderCommandList& commandList) const;
	void Release(RendererTexture& texture) const noexcept;

	static std::uint64_t CalculatePayloadBytes(const RhiTextureUploadDesc& textureUpload) noexcept;

private:
	RhiResourceService& m_resourceService;
	RhiDescriptorService& m_descriptorService;
	RhiUploadService& m_uploadService;
};
