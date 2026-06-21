#include "PCH.h"
#include "D3D12/Resources/D3D12ConstantBufferManager.h"
#include "D3D12/Resources/D3D12FrameResource.h"

#include <cstring>

D3D12ConstantBufferManager::D3D12ConstantBufferManager(D3D12FrameResourceManager& frameResourceManager) noexcept :
    m_frameResourceManager(&frameResourceManager)
{
}

D3D12ConstantBufferManager::~D3D12ConstantBufferManager() noexcept
{
}

RhiGpuVirtualAddress D3D12ConstantBufferManager::AllocateUniform(const void* data, std::uint32_t sizeInBytes)
{
	if (data == nullptr || sizeInBytes == 0)
	{
		return 0;
	}

	D3D12LinearAllocation allocation = m_frameResourceManager->GetCurrentAllocator().Allocate(sizeInBytes, 256);
	std::memcpy(allocation.CpuPtr, data, sizeInBytes);
	return allocation.GpuAddress;
}

