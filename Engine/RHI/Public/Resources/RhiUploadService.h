#pragma once

#include "../Resources/RhiResourceDesc.h"
#include "../Resources/RhiResourceHandles.h"
#include "../Resources/RhiTextureUpload.h"
#include "../RHIAPI.h"

#include <cstdint>
#include <string_view>

class RenderCommandList;

class SPARKLE_RHI_API RhiUploadService
{
  public:
	virtual ~RhiUploadService() noexcept;

	virtual RhiGpuVirtualAddress AllocateUniformConstantBuffer(
	    RenderCommandList& commandList,
	    const void* data,
	    std::uint32_t sizeInBytes) = 0;
	virtual bool UploadTexture(
	    RenderCommandList& commandList,
	    RhiOwnedResourceHandle destination,
	    const RhiTextureUploadDesc& textureUpload,
	    ResourceState finalState,
	    std::wstring_view debugName) = 0;
};
