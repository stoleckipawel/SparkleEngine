#pragma once

#include "../RHIAPI.h"

#include <d3d12.h>

class SPARKLE_RHI_API Texture
{
  public:
	virtual ~Texture() noexcept = default;

	virtual void WriteShaderResourceView(D3D12_CPU_DESCRIPTOR_HANDLE destination) const = 0;
};