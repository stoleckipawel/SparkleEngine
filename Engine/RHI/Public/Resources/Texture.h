#pragma once

#include "../Descriptors/RhiDescriptorHandles.h"
#include "../RHIAPI.h"
#include "Resources/TextureTypes.h"

class SPARKLE_RHI_API Texture
{
  public:
	virtual ~Texture() noexcept = default;

	virtual void WriteShaderResourceView(RhiCpuDescriptorHandle destination) const = 0;
	virtual TextureRuntimeInfo GetRuntimeInfo() const noexcept { return {}; }
};