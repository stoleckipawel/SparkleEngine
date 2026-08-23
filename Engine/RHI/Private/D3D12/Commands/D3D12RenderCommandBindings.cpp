#include "PCH.h"

#include "D3D12/Commands/D3D12RenderCommandList.h"

#include "D3D12/D3D12RenderHardwareInterface.h"
#include "D3D12/D3D12TypeConversions.h"

void D3D12RenderCommandList::BindGraphicsConstantBuffer(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->SetGraphicsRootConstantBufferView(bindingIndex, gpuAddress);
	}
}

void D3D12RenderCommandList::BindGraphicsShaderResource(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->SetGraphicsRootShaderResourceView(bindingIndex, gpuAddress);
	}
}

void D3D12RenderCommandList::BindGraphicsUnorderedAccess(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->SetGraphicsRootUnorderedAccessView(bindingIndex, gpuAddress);
	}
}

void D3D12RenderCommandList::BindGraphicsAccelerationStructure(std::uint32_t bindingIndex, RhiResourceHandle resource) noexcept
{
	ID3D12Resource* const nativeResource = D3D12TypeConversions::ToResource(resource);
	if (m_commandList != nullptr && nativeResource != nullptr)
	{
		m_commandList->SetGraphicsRootShaderResourceView(bindingIndex, nativeResource->GetGPUVirtualAddress());
	}
}

void D3D12RenderCommandList::BindGraphicsDescriptorTable(std::uint32_t bindingIndex, RhiDescriptorTableBinding tableBinding) noexcept
{
	if (m_commandList == nullptr || m_owner == nullptr || !tableBinding)
	{
		return;
	}

	m_commandList->SetGraphicsRootDescriptorTable(
	    bindingIndex,
	    m_owner->ResolveDescriptorTableGpuHandle(tableBinding.Table, tableBinding.DescriptorIndex));
}

void D3D12RenderCommandList::BindGraphicsDescriptorTable(std::uint32_t bindingIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->SetGraphicsRootDescriptorTable(bindingIndex, D3D12TypeConversions::ToGpuDescriptor(baseDescriptor));
	}
}

void D3D12RenderCommandList::SetGraphicsPushConstants(
    std::uint32_t bindingIndex,
    std::uint32_t num32BitValues,
    const void* data,
    std::uint32_t destOffsetIn32BitValues) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->SetGraphicsRoot32BitConstants(bindingIndex, num32BitValues, data, destOffsetIn32BitValues);
	}
}

void D3D12RenderCommandList::BindComputeConstantBuffer(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->SetComputeRootConstantBufferView(bindingIndex, gpuAddress);
	}
}

void D3D12RenderCommandList::BindComputeShaderResource(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->SetComputeRootShaderResourceView(bindingIndex, gpuAddress);
	}
}

void D3D12RenderCommandList::BindComputeUnorderedAccess(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->SetComputeRootUnorderedAccessView(bindingIndex, gpuAddress);
	}
}

void D3D12RenderCommandList::BindComputeAccelerationStructure(std::uint32_t bindingIndex, RhiResourceHandle resource) noexcept
{
	ID3D12Resource* const nativeResource = D3D12TypeConversions::ToResource(resource);
	if (m_commandList != nullptr && nativeResource != nullptr)
	{
		m_commandList->SetComputeRootShaderResourceView(bindingIndex, nativeResource->GetGPUVirtualAddress());
	}
}

void D3D12RenderCommandList::BindComputeDescriptorTable(std::uint32_t bindingIndex, RhiDescriptorTableBinding tableBinding) noexcept
{
	if (m_commandList == nullptr || m_owner == nullptr || !tableBinding)
	{
		return;
	}

	m_commandList->SetComputeRootDescriptorTable(
	    bindingIndex,
	    m_owner->ResolveDescriptorTableGpuHandle(tableBinding.Table, tableBinding.DescriptorIndex));
}

void D3D12RenderCommandList::BindComputeDescriptorTable(std::uint32_t bindingIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->SetComputeRootDescriptorTable(bindingIndex, D3D12TypeConversions::ToGpuDescriptor(baseDescriptor));
	}
}

void D3D12RenderCommandList::SetComputePushConstants(
    std::uint32_t bindingIndex,
    std::uint32_t num32BitValues,
    const void* data,
    std::uint32_t destOffsetIn32BitValues) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->SetComputeRoot32BitConstants(bindingIndex, num32BitValues, data, destOffsetIn32BitValues);
	}
}
