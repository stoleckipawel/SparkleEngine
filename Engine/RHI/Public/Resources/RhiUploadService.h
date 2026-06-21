#pragma once

#include "../Resources/RhiResourceDesc.h"
#include "../RHIAPI.h"

#include <cstdint>

class SPARKLE_RHI_API RhiUploadService
{
  public:
	virtual ~RhiUploadService() noexcept = default;

	virtual RhiGpuVirtualAddress AllocateUniformConstantBuffer(const void* data, std::uint32_t sizeInBytes) = 0;
};
