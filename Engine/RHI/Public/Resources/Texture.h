#pragma once

#include "../Interop/RenderHardwareInterface.h"

class SPARKLE_RHI_API Texture
{
  public:
	virtual ~Texture() noexcept = default;

	virtual void WriteShaderResourceView(RhiCpuDescriptorHandle destination) const = 0;
};