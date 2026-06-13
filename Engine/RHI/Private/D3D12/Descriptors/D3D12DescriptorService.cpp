#include "D3D12/Descriptors/D3D12DescriptorService.h"

#include "D3D12/D3D12TypeConversions.h"
#include "D3D12/Commands/D3D12RenderCommandList.h"
#include "D3D12/Descriptors/D3D12DescriptorHeap.h"
#include "D3D12/Descriptors/D3D12DescriptorHeapManager.h"
#include "D3D12/Device/D3D12Rhi.h"
#include "D3D12/Samplers/D3D12SamplerLibrary.h"

#include <d3d12.h>

D3D12DescriptorService::D3D12DescriptorService(
    D3D12Rhi& rhi,
    D3D12DescriptorHeapManager& descriptorHeapManager,
    const RhiCapabilities& capabilities) noexcept :
	m_rhi(&rhi), m_descriptorHeapManager(&descriptorHeapManager), m_capabilities(&capabilities)
{
}

ID3D12DescriptorHeap* D3D12DescriptorService::GetShaderResourceDescriptorHeap() const noexcept
{
	if (m_descriptorHeapManager == nullptr)
	{
		return nullptr;
	}

	D3D12DescriptorHeap* heap = m_descriptorHeapManager->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	return heap != nullptr ? heap->GetRaw() : nullptr;
}

std::unique_ptr<RenderBindingSet> D3D12DescriptorService::CreateBindingSet(const RenderBindingSetDesc& desc)
{
	return m_capabilities != nullptr ? std::make_unique<RenderBindingSet>(*m_capabilities, *this, desc) :
	                                   std::unique_ptr<RenderBindingSet>{};
}

void D3D12DescriptorService::BindGlobalDescriptorState(RenderCommandList& commandList) const noexcept
{
	if (m_descriptorHeapManager != nullptr && commandList.GetBackendApi() == ERhiBackendApi::D3D12)
	{
		m_descriptorHeapManager->BindGlobalDescriptorState(static_cast<D3D12RenderCommandList&>(commandList));
	}
}

RhiDescriptorAllocation D3D12DescriptorService::AllocateDescriptor(ERhiDescriptorAllocatorType descriptorType)
{
	RhiDescriptorAllocation allocation{};
	if (m_descriptorHeapManager == nullptr)
	{
		return allocation;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};
	m_descriptorHeapManager->AllocateHandle(D3D12TypeConversions::ToDescriptorHeapType(descriptorType), cpuHandle, gpuHandle);
	allocation.CpuHandle = RhiCpuDescriptorHandle{cpuHandle.ptr};
	allocation.GpuHandle = RhiGpuDescriptorHandle{gpuHandle.ptr};
	return allocation;
}

void D3D12DescriptorService::ReleaseDescriptor(
    ERhiDescriptorAllocatorType descriptorType,
    const RhiDescriptorAllocation& allocation) noexcept
{
	if (m_descriptorHeapManager == nullptr || !allocation.CpuHandle)
	{
		return;
	}

	m_descriptorHeapManager->FreeHandle(
	    D3D12TypeConversions::ToDescriptorHeapType(descriptorType),
	    D3D12_CPU_DESCRIPTOR_HANDLE{allocation.CpuHandle.Value},
	    D3D12_GPU_DESCRIPTOR_HANDLE{allocation.GpuHandle.Value});
}

RhiDescriptorTableHandle D3D12DescriptorService::AllocateDescriptorTable(
    ERhiDescriptorAllocatorType descriptorType,
    std::uint32_t descriptorCount)
{
	if (m_descriptorHeapManager == nullptr || descriptorCount == 0)
	{
		return {};
	}

	const D3D12DescriptorHandle nativeHandle =
	    m_descriptorHeapManager->AllocateContiguous(D3D12TypeConversions::ToDescriptorHeapType(descriptorType), descriptorCount);
	if (!nativeHandle.IsValid())
	{
		return {};
	}

	DescriptorTableRecord record{};
	record.descriptorType = descriptorType;
	record.descriptorCount = descriptorCount;
	record.nativeHandle = nativeHandle;

	if (!m_freeDescriptorTableIndices.empty())
	{
		const std::uint32_t recordIndex = m_freeDescriptorTableIndices.back();
		m_freeDescriptorTableIndices.pop_back();
		m_descriptorTableRecords[recordIndex] = record;
		return RhiDescriptorTableHandle{recordIndex + 1u};
	}

	m_descriptorTableRecords.push_back(record);
	return RhiDescriptorTableHandle{static_cast<std::uint32_t>(m_descriptorTableRecords.size())};
}

RhiCpuDescriptorHandle D3D12DescriptorService::GetDescriptorTableCpuHandle(
    RhiDescriptorTableHandle tableHandle,
    std::uint32_t descriptorIndex) const noexcept
{
	const DescriptorTableRecord* const record = FindDescriptorTableRecord(tableHandle);
	if (record == nullptr || descriptorIndex >= record->descriptorCount)
	{
		return {};
	}

	D3D12_CPU_DESCRIPTOR_HANDLE nativeHandle = record->nativeHandle.GetCPU();
	nativeHandle.ptr += static_cast<SIZE_T>(descriptorIndex) * record->nativeHandle.GetIncrementSize();
	return RhiCpuDescriptorHandle{nativeHandle.ptr};
}

RhiGpuDescriptorHandle D3D12DescriptorService::GetDescriptorTableGpuHandle(
    RhiDescriptorTableHandle tableHandle,
    std::uint32_t descriptorIndex) const noexcept
{
	const DescriptorTableRecord* const record = FindDescriptorTableRecord(tableHandle);
	if (record == nullptr || descriptorIndex >= record->descriptorCount)
	{
		return {};
	}

	D3D12_GPU_DESCRIPTOR_HANDLE nativeHandle = record->nativeHandle.GetGPU();
	nativeHandle.ptr += static_cast<UINT64>(descriptorIndex) * record->nativeHandle.GetIncrementSize();
	return RhiGpuDescriptorHandle{nativeHandle.ptr};
}

void D3D12DescriptorService::ReleaseDescriptorTable(RhiDescriptorTableHandle tableHandle) noexcept
{
	DescriptorTableRecord* const record = FindDescriptorTableRecord(tableHandle);
	if (record == nullptr || m_descriptorHeapManager == nullptr || !record->IsAllocated())
	{
		return;
	}

	m_descriptorHeapManager->FreeContiguous(
	    D3D12TypeConversions::ToDescriptorHeapType(record->descriptorType),
	    record->nativeHandle,
	    record->descriptorCount);
	*record = DescriptorTableRecord{};
	m_freeDescriptorTableIndices.push_back(tableHandle.Value - 1u);
}

void D3D12DescriptorService::AllocateShaderResourceDescriptor(
    RhiCpuDescriptorHandle& outCpuHandle,
    RhiGpuDescriptorHandle& outGpuHandle)
{
	outCpuHandle = {};
	outGpuHandle = {};
	if (m_descriptorHeapManager == nullptr)
	{
		return;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};
	m_descriptorHeapManager->AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, cpuHandle, gpuHandle);
	outCpuHandle.Value = cpuHandle.ptr;
	outGpuHandle.Value = gpuHandle.ptr;
}

void D3D12DescriptorService::ReleaseShaderResourceDescriptor(
    RhiCpuDescriptorHandle cpuHandle,
    RhiGpuDescriptorHandle gpuHandle) noexcept
{
	if (m_descriptorHeapManager == nullptr || !cpuHandle)
	{
		return;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE nativeCpuHandle{};
	nativeCpuHandle.ptr = cpuHandle.Value;
	D3D12_GPU_DESCRIPTOR_HANDLE nativeGpuHandle{};
	nativeGpuHandle.ptr = gpuHandle.Value;
	m_descriptorHeapManager->FreeHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, nativeCpuHandle, nativeGpuHandle);
}

RhiDescriptorTableBinding D3D12DescriptorService::GetSharedSamplerBinding(const RhiSamplerDesc& samplerDesc) const noexcept
{
	D3D12SamplerLibrary::Slot slot = D3D12SamplerLibrary::Slot::Count;
	if (!m_samplerTableHandle || !D3D12SamplerLibrary::TryGetSlot(samplerDesc, slot))
	{
		return {};
	}

	return RhiDescriptorTableBinding{m_samplerTableHandle, static_cast<std::uint32_t>(slot)};
}

void D3D12DescriptorService::SetSamplerTableHandle(RhiDescriptorTableHandle samplerTableHandle) noexcept
{
	m_samplerTableHandle = samplerTableHandle;
}

RhiResourceViewHandle D3D12DescriptorService::CreateResourceView(const RhiResourceViewDesc& desc)
{
	const ERhiDescriptorAllocatorType descriptorType = ResolveResourceViewDescriptorAllocatorType(desc.Kind);
	RhiDescriptorAllocation allocation = AllocateDescriptor(descriptorType);
	if (!allocation.IsValid())
	{
		return {};
	}

	if (!WriteResourceViewDescriptor(desc, allocation.CpuHandle))
	{
		ReleaseDescriptor(descriptorType, allocation);
		return {};
	}

	ResourceViewRecord record{};
	record.kind = desc.Kind;
	record.descriptorType = descriptorType;
	record.descriptorAllocation = allocation;

	if (!m_freeResourceViewIndices.empty())
	{
		const std::uint32_t recordIndex = m_freeResourceViewIndices.back();
		m_freeResourceViewIndices.pop_back();
		m_resourceViewRecords[recordIndex] = record;
		return RhiResourceViewHandle{recordIndex + 1u};
	}

	m_resourceViewRecords.push_back(record);
	return RhiResourceViewHandle{static_cast<std::uint32_t>(m_resourceViewRecords.size())};
}

void D3D12DescriptorService::ReleaseResourceView(RhiResourceViewHandle view) noexcept
{
	ResourceViewRecord* const record = FindResourceViewRecord(view);
	if (record == nullptr)
	{
		return;
	}

	ReleaseDescriptor(record->descriptorType, record->descriptorAllocation);
	*record = ResourceViewRecord{};
	m_freeResourceViewIndices.push_back(view.Value - 1u);
}

RhiCpuDescriptorHandle D3D12DescriptorService::GetResourceViewCpuHandle(RhiResourceViewHandle view) const noexcept
{
	const ResourceViewRecord* const record = FindResourceViewRecord(view);
	return record != nullptr ? record->descriptorAllocation.CpuHandle : RhiCpuDescriptorHandle{};
}

RhiGpuDescriptorHandle D3D12DescriptorService::GetResourceViewGpuHandle(RhiResourceViewHandle view) const noexcept
{
	const ResourceViewRecord* const record = FindResourceViewRecord(view);
	return record != nullptr ? record->descriptorAllocation.GpuHandle : RhiGpuDescriptorHandle{};
}

NativeTextureViewInfo D3D12DescriptorService::GetNativeTextureViewInfo(RhiResourceViewHandle, ResourceState) const noexcept
{
	return {};
}

ERhiDescriptorAllocatorType D3D12DescriptorService::ResolveResourceViewDescriptorAllocatorType(
    ERhiResourceViewKind kind) noexcept
{
	switch (kind)
	{
		case ERhiResourceViewKind::RenderTarget:
			return ERhiDescriptorAllocatorType::RenderTarget;
		case ERhiResourceViewKind::DepthStencil:
			return ERhiDescriptorAllocatorType::DepthStencil;
		case ERhiResourceViewKind::TextureShaderResource:
		case ERhiResourceViewKind::TextureUnorderedAccess:
		case ERhiResourceViewKind::BufferShaderResource:
		case ERhiResourceViewKind::BufferUnorderedAccess:
		case ERhiResourceViewKind::AccelerationStructureShaderResource:
		default:
			return ERhiDescriptorAllocatorType::ShaderResource;
	}
}

bool D3D12DescriptorService::WriteResourceViewDescriptor(
    const RhiResourceViewDesc& desc,
    RhiCpuDescriptorHandle destination) noexcept
{
	if (m_rhi == nullptr || !destination)
	{
		return false;
	}

	ID3D12Device* const device = m_rhi->GetDevice().Get();
	if (device == nullptr)
	{
		return false;
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE nativeDestination = D3D12TypeConversions::ToCpuDescriptor(destination);
	switch (desc.Kind)
	{
		case ERhiResourceViewKind::RenderTarget:
		{
			if (!desc.Resource)
			{
				return false;
			}

			D3D12_RENDER_TARGET_VIEW_DESC viewDesc{};
			viewDesc.Format = D3D12TypeConversions::ToDxgiFormat(desc.Format);
			viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			device->CreateRenderTargetView(D3D12TypeConversions::ToResource(desc.Resource), &viewDesc, nativeDestination);
			return true;
		}
		case ERhiResourceViewKind::DepthStencil:
		{
			if (!desc.Resource)
			{
				return false;
			}

			D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc{};
			viewDesc.Format = D3D12TypeConversions::ToDxgiFormat(desc.Format);
			viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			viewDesc.Flags = D3D12_DSV_FLAG_NONE;
			device->CreateDepthStencilView(D3D12TypeConversions::ToResource(desc.Resource), &viewDesc, nativeDestination);
			return true;
		}
		case ERhiResourceViewKind::TextureShaderResource:
		{
			if (!desc.Resource)
			{
				return false;
			}

			D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc{};
			viewDesc.Format = D3D12TypeConversions::ToDxgiFormat(desc.Format);
			viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			viewDesc.Texture2D.MostDetailedMip = desc.Texture.MostDetailedMip;
			viewDesc.Texture2D.MipLevels = desc.Texture.MipCount;
			device->CreateShaderResourceView(D3D12TypeConversions::ToResource(desc.Resource), &viewDesc, nativeDestination);
			return true;
		}
		case ERhiResourceViewKind::TextureUnorderedAccess:
		{
			if (!desc.Resource)
			{
				return false;
			}

			D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc{};
			viewDesc.Format = D3D12TypeConversions::ToDxgiFormat(desc.Format);
			viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			viewDesc.Texture2D.MipSlice = desc.Texture.MostDetailedMip;
			viewDesc.Texture2D.PlaneSlice = 0;
			device->CreateUnorderedAccessView(D3D12TypeConversions::ToResource(desc.Resource), nullptr, &viewDesc, nativeDestination);
			return true;
		}
		case ERhiResourceViewKind::BufferShaderResource:
		{
			if (!desc.Resource || desc.Buffer.SizeInBytes == 0)
			{
				return false;
			}

			D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc{};
			viewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			if (desc.Buffer.StrideInBytes > 0)
			{
				viewDesc.Format = DXGI_FORMAT_UNKNOWN;
				viewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
				viewDesc.Buffer.FirstElement = desc.Buffer.OffsetInBytes / desc.Buffer.StrideInBytes;
				viewDesc.Buffer.StructureByteStride = desc.Buffer.StrideInBytes;
				viewDesc.Buffer.NumElements = static_cast<UINT>(desc.Buffer.SizeInBytes / desc.Buffer.StrideInBytes);
			}
			else
			{
				viewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
				viewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
				viewDesc.Buffer.FirstElement = desc.Buffer.OffsetInBytes / sizeof(std::uint32_t);
				viewDesc.Buffer.StructureByteStride = 0;
				viewDesc.Buffer.NumElements = static_cast<UINT>(desc.Buffer.SizeInBytes / sizeof(std::uint32_t));
			}
			device->CreateShaderResourceView(D3D12TypeConversions::ToResource(desc.Resource), &viewDesc, nativeDestination);
			return true;
		}
		case ERhiResourceViewKind::BufferUnorderedAccess:
		{
			if (!desc.Resource || desc.Buffer.SizeInBytes == 0)
			{
				return false;
			}

			D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc{};
			viewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			if (desc.Buffer.StrideInBytes > 0)
			{
				viewDesc.Format = DXGI_FORMAT_UNKNOWN;
				viewDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
				viewDesc.Buffer.FirstElement = desc.Buffer.OffsetInBytes / desc.Buffer.StrideInBytes;
				viewDesc.Buffer.StructureByteStride = desc.Buffer.StrideInBytes;
				viewDesc.Buffer.NumElements = static_cast<UINT>(desc.Buffer.SizeInBytes / desc.Buffer.StrideInBytes);
			}
			else
			{
				viewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
				viewDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
				viewDesc.Buffer.FirstElement = desc.Buffer.OffsetInBytes / sizeof(std::uint32_t);
				viewDesc.Buffer.StructureByteStride = 0;
				viewDesc.Buffer.NumElements = static_cast<UINT>(desc.Buffer.SizeInBytes / sizeof(std::uint32_t));
			}
			device->CreateUnorderedAccessView(D3D12TypeConversions::ToResource(desc.Resource), nullptr, &viewDesc, nativeDestination);
			return true;
		}
		case ERhiResourceViewKind::AccelerationStructureShaderResource:
		{
			if (desc.AccelerationStructureGpuAddress == 0)
			{
				return false;
			}

			D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc{};
			viewDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
			viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			viewDesc.RaytracingAccelerationStructure.Location = desc.AccelerationStructureGpuAddress;
			device->CreateShaderResourceView(nullptr, &viewDesc, nativeDestination);
			return true;
		}
		default:
			return false;
	}
}

D3D12DescriptorService::DescriptorTableRecord* D3D12DescriptorService::FindDescriptorTableRecord(
    RhiDescriptorTableHandle tableHandle) noexcept
{
	if (!tableHandle || tableHandle.Value == 0 || tableHandle.Value > m_descriptorTableRecords.size())
	{
		return nullptr;
	}

	DescriptorTableRecord& record = m_descriptorTableRecords[tableHandle.Value - 1u];
	return record.IsAllocated() ? &record : nullptr;
}

const D3D12DescriptorService::DescriptorTableRecord* D3D12DescriptorService::FindDescriptorTableRecord(
    RhiDescriptorTableHandle tableHandle) const noexcept
{
	if (!tableHandle || tableHandle.Value == 0 || tableHandle.Value > m_descriptorTableRecords.size())
	{
		return nullptr;
	}

	const DescriptorTableRecord& record = m_descriptorTableRecords[tableHandle.Value - 1u];
	return record.IsAllocated() ? &record : nullptr;
}

D3D12DescriptorService::ResourceViewRecord* D3D12DescriptorService::FindResourceViewRecord(RhiResourceViewHandle view) noexcept
{
	if (!view || view.Value == 0 || view.Value > m_resourceViewRecords.size())
	{
		return nullptr;
	}

	ResourceViewRecord& record = m_resourceViewRecords[view.Value - 1u];
	return record.IsAllocated() ? &record : nullptr;
}

const D3D12DescriptorService::ResourceViewRecord* D3D12DescriptorService::FindResourceViewRecord(
    RhiResourceViewHandle view) const noexcept
{
	if (!view || view.Value == 0 || view.Value > m_resourceViewRecords.size())
	{
		return nullptr;
	}

	const ResourceViewRecord& record = m_resourceViewRecords[view.Value - 1u];
	return record.IsAllocated() ? &record : nullptr;
}
