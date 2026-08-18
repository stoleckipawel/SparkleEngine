#include "../PCH.h"
#include "Textures/RendererTextureFactory.h"

#include "Core/Public/Diagnostics/Verify.h"
#include "RHI/Public/Commands/RenderCommandList.h"
#include "RHI/Public/Descriptors/RhiDescriptorService.h"
#include "RHI/Public/Resources/RhiResourceService.h"
#include "RHI/Public/Resources/RhiUploadService.h"
#include "Textures/CookedTextureLoader.h"

#include <format>

static const auto g_rendererTextureFactoryLogger = Logging::GetOrCreateLogger("Renderer.RendererTextureFactory");

RendererTextureFactory::RendererTextureFactory(
    RhiResourceService& resourceService,
    RhiDescriptorService& descriptorService,
    RhiUploadService& uploadService) noexcept :
    m_resourceService(resourceService),
    m_descriptorService(descriptorService),
    m_uploadService(uploadService)
{
}

RendererTexture RendererTextureFactory::Create(
    const std::filesystem::path& texturePath,
    LoadedTextureData& loadedTexture,
    RenderCommandList& commandList) const
{
	const RhiTextureUploadDesc& textureUpload = loadedTexture.Upload;
	const std::wstring debugName = texturePath.filename().wstring();
	const RhiTextureResourceDesc resourceDesc{
	    .Width = textureUpload.Width,
	    .Height = textureUpload.Height,
	    .Format = textureUpload.Format,
	    .MipLevels = textureUpload.GetMipCount(),
	    .ArraySize = textureUpload.GetArraySize(),
	    .Dimension = textureUpload.Dimension};
	RhiOwnedResourceHandle resource = m_resourceService.CreateTextureResource(
	    resourceDesc,
	    ResourceState::CopyDest,
	    RhiMemoryCategory::Texture,
	    RhiMemoryResidencyClass::DeviceLocal,
	    debugName);
	if (!resource)
	{
		Diagnostics::Fatal(
		    g_rendererTextureFactoryLogger,
		    __FILE__,
		    __LINE__,
		    std::format("Texture resource creation failed for '{}'.", texturePath.string()));
	}

	if (!m_uploadService.UploadTexture(commandList, resource, textureUpload, ResourceState::ShaderResource, debugName))
	{
		m_resourceService.ReleaseOwnedResource(resource);
		Diagnostics::Fatal(
		    g_rendererTextureFactoryLogger,
		    __FILE__,
		    __LINE__,
		    std::format("Texture upload failed for '{}'.", texturePath.string()));
	}

	const RhiResourceHandle nativeResource = m_resourceService.GetResourceHandle(resource);
	RhiResourceViewHandle shaderResourceView = m_descriptorService.CreateResourceView(
	    RhiResourceViewDesc::TextureShaderResource(
	        nativeResource,
	        textureUpload.Format,
	        RhiTextureViewRange{
	            .MostDetailedMip = 0,
	            .MipCount = textureUpload.GetMipCount(),
	            .FirstArraySlice = 0,
	            .ArraySize = textureUpload.GetArraySize()},
	        textureUpload.Dimension));
	if (!shaderResourceView)
	{
		m_resourceService.ReleaseOwnedResource(resource);
		Diagnostics::Fatal(
		    g_rendererTextureFactoryLogger,
		    __FILE__,
		    __LINE__,
		    std::format("Texture shader-resource view creation failed for '{}'.", texturePath.string()));
	}

	return RendererTexture{
	    .Resource = resource,
	    .ShaderResourceView = shaderResourceView,
	    .Width = textureUpload.Width,
	    .Height = textureUpload.Height,
	    .ArraySize = textureUpload.ArraySize,
	    .Dimension = textureUpload.Dimension,
	    .Format = textureUpload.Format,
	    .FormatIntent = loadedTexture.FormatIntent,
	    .MipCount = textureUpload.GetMipCount(),
	    .EstimatedByteSize = CalculatePayloadBytes(textureUpload)};
}

void RendererTextureFactory::Release(RendererTexture& texture) const noexcept
{
	if (texture.ShaderResourceView)
	{
		m_descriptorService.ReleaseResourceView(texture.ShaderResourceView);
	}
	if (texture.Resource)
	{
		m_resourceService.ReleaseOwnedResource(texture.Resource);
	}
	texture = {};
}

std::uint64_t RendererTextureFactory::CalculatePayloadBytes(const RhiTextureUploadDesc& textureUpload) noexcept
{
	std::uint64_t byteCount = 0;
	for (const RhiTextureArraySliceUploadData& arraySlice : textureUpload.ArraySlices)
	{
		for (const RhiTextureMipUploadData& mipLevel : arraySlice.MipLevels)
		{
			byteCount += mipLevel.Data.size();
		}
	}
	return byteCount;
}
