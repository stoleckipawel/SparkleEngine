#pragma once

#include "D3D12/Memory/D3D12GpuAllocation.h"

#include <cstddef>
#include <memory>
#include <type_traits>

class D3D12Rhi;

class D3D12UploadBuffer
{
public:
	static std::unique_ptr<D3D12GpuAllocationRecord> Upload(D3D12Rhi& rhi, const void* data, size_t dataSize);
};
