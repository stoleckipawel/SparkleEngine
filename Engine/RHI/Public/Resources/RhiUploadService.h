#pragma once

#include "../Resources/PerFrameConstantBufferData.h"
#include "../Resources/PerObjectConstantBufferData.h"
#include "../Resources/PerViewConstantBufferData.h"
#include "../Resources/RhiResourceDesc.h"
#include "../RHIAPI.h"

#include <cstdint>

class SPARKLE_RHI_API RhiUploadService
{
  public:
	virtual ~RhiUploadService() noexcept = default;

	virtual const PerFrameConstantBufferData& GetPerFrameConstantData() const noexcept = 0;
	virtual RhiGpuVirtualAddress GetPerFrameConstantGpuAddress() const noexcept = 0;
	virtual RhiGpuVirtualAddress AllocateUniformConstantBuffer(const void* data, std::uint32_t sizeInBytes) = 0;
	virtual RhiGpuVirtualAddress AllocatePerViewConstantBuffer(const PerViewConstantBufferData& data) = 0;
	virtual RhiGpuVirtualAddress AllocatePerObjectVertexConstants(const PerObjectVSConstantBufferData& data) = 0;
	virtual RhiGpuVirtualAddress AllocatePerObjectPixelConstants(const PerObjectPSConstantBufferData& data) = 0;
};
