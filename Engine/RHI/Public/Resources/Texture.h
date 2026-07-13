#pragma once

#include "../Descriptors/RhiDescriptorHandles.h"
#include "../RHIAPI.h"
#include "RHI/Public/Interop/RhiNativeHandles.h"
#include "Resources/TextureTypes.h"

class SPARKLE_RHI_API Texture
{
  public:
	virtual ~Texture() noexcept = default;

	virtual NativeResourceHandle GetNativeResource() const noexcept = 0;
	virtual void WriteShaderResourceView(RhiCpuDescriptorHandle destination) const = 0;
	virtual TextureRuntimeInfo GetRuntimeInfo() const noexcept { return {}; }
};
