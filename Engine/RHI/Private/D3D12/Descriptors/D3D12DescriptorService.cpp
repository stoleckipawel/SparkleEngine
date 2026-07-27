#include "D3D12/Descriptors/D3D12DescriptorService.h"

#include "D3D12/D3D12TypeConversions.h"
#include "D3D12/Commands/D3D12RenderCommandList.h"
#include "D3D12/Descriptors/D3D12DescriptorHeap.h"
#include "D3D12/Descriptors/D3D12DescriptorHeapManager.h"
#include "D3D12/Device/D3D12Rhi.h"
#include "D3D12/Samplers/D3D12SamplerLibrary.h"
#include "Validation/RhiContract.h"

#include <d3d12.h>

class D3D12DescriptorFormatSelection final
{
  public:
	static DXGI_FORMAT ResolveTextureShaderResourceViewFormat(PixelFormat format) noexcept
	{
		switch (format)
		{
			case PixelFormat::D32_Float:
				return DXGI_FORMAT_R32_FLOAT;
			case PixelFormat::D24_UNorm_S8_UInt:
				return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			default:
				return D3D12TypeConversions::ToDxgiFormat(format);
		}
	}
};

D3D12DescriptorService::D3D12DescriptorService(
    D3D12Rhi& rhi,
    D3D12DescriptorHeapManager& descriptorHeapManager,
    const RhiCapabilities& capabilities) noexcept :
	m_rhi(&rhi), m_descriptorHeapManager(&descriptorHeapManager), m_capabilities(&capabilities)
{
}

D3D12DescriptorService::~D3D12DescriptorService() noexcept
{
	ReleaseAllDescriptors();
}

void D3D12DescriptorService::BeginFrame(std::uint32_t frameIndex) noexcept
{
	if (frameIndex >= m_retiredResourceViews.size())
	{
		return;
	}

	for (const RetiredDescriptorAllocation& retired : m_retiredDescriptorAllocations[frameIndex])
	{
		DestroyDescriptorAllocation(retired.descriptorType, retired.allocation);
	}
	m_retiredDescriptorAllocations[frameIndex].clear();

	for (const RetiredDescriptorTable& retired : m_retiredDescriptorTables[frameIndex])
	{
		DestroyDescriptorTable(retired.descriptorType, retired.nativeHandle, retired.descriptorCount);
		RecycleDescriptorTableRecord(retired.recordIndex);
	}
	m_retiredDescriptorTables[frameIndex].clear();

	for (RetiredResourceView& retired : m_retiredResourceViews[frameIndex])
	{
		DestroyResourceView(retired.record);
		RecycleResourceViewRecord(retired.recordIndex);
	}
	m_retiredResourceViews[frameIndex].clear();
	m_currentFrameIndex = frameIndex;
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
	if (m_descriptorHeapManager == nullptr || !allocation.IsValid())
	{
		return;
	}

	m_retiredDescriptorAllocations[m_currentFrameIndex].push_back(
	    RetiredDescriptorAllocation{.descriptorType = descriptorType, .allocation = allocation});
}

RhiDescriptorTableHandle D3D12DescriptorService::AllocateDescriptorTable(
    ERhiDescriptorAllocatorType descriptorType,
    std::uint32_t descriptorCount)
{
	if (m_descriptorHeapManager == nullptr || descriptorCount == 0 ||
	    (m_freeDescriptorTableIndices.empty() &&
	     m_descriptorTableRecords.size() >= RhiDescriptorTableHandle::MaximumRecordCount))
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
		const std::uint16_t generation = m_descriptorTableRecords[recordIndex].generation;
		m_descriptorTableRecords[recordIndex] = record;
		m_descriptorTableRecords[recordIndex].generation = generation;
		return RhiDescriptorTableHandle::Make(recordIndex, generation);
	}

	m_descriptorTableRecords.push_back(record);
	return RhiDescriptorTableHandle::Make(static_cast<std::uint32_t>(m_descriptorTableRecords.size() - 1u), 0u);
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

	std::uint32_t recordIndex = 0;
	std::uint16_t generation = 0;
	if (!tableHandle.Decode(recordIndex, generation))
	{
		return;
	}

	m_retiredDescriptorTables[m_currentFrameIndex].push_back(
	    RetiredDescriptorTable{
	        .descriptorType = record->descriptorType,
	        .descriptorCount = record->descriptorCount,
	        .nativeHandle = record->nativeHandle,
	        .recordIndex = recordIndex});
	record->descriptorCount = 0;
	record->nativeHandle = {};
	if (m_samplerTableHandle == tableHandle)
	{
		m_samplerTableHandle = {};
	}
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
	if (!RhiContract::IsResourceViewDescUsable(desc) || m_rhi == nullptr || m_descriptorHeapManager == nullptr ||
	    (m_freeResourceViewIndices.empty() &&
	     m_resourceViewRecords.size() >= RhiResourceViewHandle::MaximumRecordCount))
	{
		return {};
	}

	const ERhiDescriptorAllocatorType descriptorType = ResolveResourceViewDescriptorAllocatorType(desc.Kind);
	RhiDescriptorAllocation allocation = AllocateDescriptor(descriptorType);
	if (!allocation.IsValid())
	{
		return {};
	}

	D3D12DescriptorHandle copySourceHandle;
	RhiCpuDescriptorHandle descriptorWriteTarget = allocation.CpuHandle;
	if (descriptorType == ERhiDescriptorAllocatorType::ShaderResource)
	{
		copySourceHandle = m_descriptorHeapManager->AllocateResourceViewCopySource();
		if (!copySourceHandle.IsValid())
		{
			DestroyDescriptorAllocation(descriptorType, allocation);
			return {};
		}
		descriptorWriteTarget = RhiCpuDescriptorHandle{copySourceHandle.GetCPU().ptr};
	}

	if (!WriteResourceViewDescriptor(desc, descriptorWriteTarget))
	{
		m_descriptorHeapManager->FreeResourceViewCopySource(copySourceHandle);
		DestroyDescriptorAllocation(descriptorType, allocation);
		return {};
	}
	if (copySourceHandle.IsValid())
	{
		m_rhi->GetDevice()->CopyDescriptorsSimple(
		    1,
		    D3D12_CPU_DESCRIPTOR_HANDLE{allocation.CpuHandle.Value},
		    copySourceHandle.GetCPU(),
		    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	ResourceViewRecord record{};
	record.kind = desc.Kind;
	record.descriptorType = descriptorType;
	record.descriptorAllocation = allocation;
	record.copySourceHandle = copySourceHandle;

	if (!m_freeResourceViewIndices.empty())
	{
		const std::uint32_t recordIndex = m_freeResourceViewIndices.back();
		m_freeResourceViewIndices.pop_back();
		const std::uint16_t generation = m_resourceViewRecords[recordIndex].generation;
		m_resourceViewRecords[recordIndex] = record;
		m_resourceViewRecords[recordIndex].generation = generation;
		return RhiResourceViewHandle::Make(recordIndex, generation);
	}
	m_resourceViewRecords.push_back(record);
	return RhiResourceViewHandle::Make(static_cast<std::uint32_t>(m_resourceViewRecords.size() - 1u), 0u);
}

bool D3D12DescriptorService::WriteResourceView(
    RhiDescriptorTableHandle tableHandle,
    std::uint32_t descriptorIndex,
    RhiResourceViewHandle view) noexcept
{
	const DescriptorTableRecord* const table = FindDescriptorTableRecord(tableHandle);
	const ResourceViewRecord* const resourceView = FindResourceViewRecord(view);
	if (m_rhi == nullptr || table == nullptr || resourceView == nullptr || descriptorIndex >= table->descriptorCount ||
	    table->descriptorType != resourceView->descriptorType)
	{
		return false;
	}

	ID3D12Device* const device = m_rhi->GetDevice().Get();
	if (device == nullptr)
	{
		return false;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE destination = table->nativeHandle.GetCPU();
	destination.ptr += static_cast<SIZE_T>(descriptorIndex) * table->nativeHandle.GetIncrementSize();
	const D3D12_CPU_DESCRIPTOR_HANDLE source = resourceView->copySourceHandle.IsValid()
	                                                   ? resourceView->copySourceHandle.GetCPU()
	                                                   : D3D12_CPU_DESCRIPTOR_HANDLE{resourceView->descriptorAllocation.CpuHandle.Value};
	device->CopyDescriptorsSimple(
	    1,
	    destination,
	    source,
	    D3D12TypeConversions::ToDescriptorHeapType(table->descriptorType));
	return true;
}

void D3D12DescriptorService::ReleaseResourceView(RhiResourceViewHandle view) noexcept
{
	ResourceViewRecord* const record = FindResourceViewRecord(view);
	if (record == nullptr)
	{
		return;
	}

	std::uint32_t recordIndex = 0;
	std::uint16_t generation = 0;
	if (!view.Decode(recordIndex, generation))
	{
		return;
	}

	m_retiredResourceViews[m_currentFrameIndex].push_back(RetiredResourceView{.record = *record, .recordIndex = recordIndex});
	record->descriptorAllocation = {};
	record->copySourceHandle = {};
}

void D3D12DescriptorService::DestroyResourceView(ResourceViewRecord& record) noexcept
{
	if (record.IsAllocated())
	{
		DestroyDescriptorAllocation(record.descriptorType, record.descriptorAllocation);
		record.descriptorAllocation = {};
	}
	if (record.copySourceHandle.IsValid())
	{
		m_descriptorHeapManager->FreeResourceViewCopySource(record.copySourceHandle);
		record.copySourceHandle = {};
	}
}

void D3D12DescriptorService::DestroyDescriptorAllocation(
    ERhiDescriptorAllocatorType descriptorType,
    const RhiDescriptorAllocation& allocation) noexcept
{
	if (m_descriptorHeapManager == nullptr || !allocation.IsValid())
	{
		return;
	}

	m_descriptorHeapManager->FreeHandle(
	    D3D12TypeConversions::ToDescriptorHeapType(descriptorType),
	    D3D12_CPU_DESCRIPTOR_HANDLE{allocation.CpuHandle.Value},
	    D3D12_GPU_DESCRIPTOR_HANDLE{allocation.GpuHandle.Value});
}

void D3D12DescriptorService::DestroyDescriptorTable(
    ERhiDescriptorAllocatorType descriptorType,
    const D3D12DescriptorHandle& nativeHandle,
    std::uint32_t descriptorCount) noexcept
{
	if (m_descriptorHeapManager != nullptr && nativeHandle.IsValid())
	{
		m_descriptorHeapManager->FreeContiguous(
		    D3D12TypeConversions::ToDescriptorHeapType(descriptorType),
		    nativeHandle,
		    descriptorCount);
	}
}

void D3D12DescriptorService::RecycleDescriptorTableRecord(std::uint32_t recordIndex) noexcept
{
	if (recordIndex >= m_descriptorTableRecords.size())
	{
		return;
	}

	DescriptorTableRecord& record = m_descriptorTableRecords[recordIndex];
	if (record.IsAllocated() || record.generation == RhiDescriptorTableHandle::MaximumGeneration)
	{
		return;
	}

	++record.generation;
	m_freeDescriptorTableIndices.push_back(recordIndex);
}

void D3D12DescriptorService::RecycleResourceViewRecord(std::uint32_t recordIndex) noexcept
{
	if (recordIndex >= m_resourceViewRecords.size())
	{
		return;
	}

	ResourceViewRecord& record = m_resourceViewRecords[recordIndex];
	if (record.IsAllocated() || record.generation == RhiResourceViewHandle::MaximumGeneration)
	{
		return;
	}

	++record.generation;
	m_freeResourceViewIndices.push_back(recordIndex);
}

void D3D12DescriptorService::ReleaseAllDescriptors() noexcept
{
	for (auto& retiredAllocations : m_retiredDescriptorAllocations)
	{
		for (const RetiredDescriptorAllocation& retired : retiredAllocations)
		{
			DestroyDescriptorAllocation(retired.descriptorType, retired.allocation);
		}
		retiredAllocations.clear();
	}

	for (DescriptorTableRecord& record : m_descriptorTableRecords)
	{
		DestroyDescriptorTable(record.descriptorType, record.nativeHandle, record.descriptorCount);
		record.nativeHandle = {};
		record.descriptorCount = 0;
	}
	for (auto& retiredTables : m_retiredDescriptorTables)
	{
		for (const RetiredDescriptorTable& table : retiredTables)
		{
			DestroyDescriptorTable(table.descriptorType, table.nativeHandle, table.descriptorCount);
		}
		retiredTables.clear();
	}

	for (ResourceViewRecord& record : m_resourceViewRecords)
	{
		DestroyResourceView(record);
	}
	for (auto& retiredViews : m_retiredResourceViews)
	{
		for (RetiredResourceView& retired : retiredViews)
		{
			DestroyResourceView(retired.record);
		}
		retiredViews.clear();
	}

	m_freeDescriptorTableIndices.clear();
	m_freeResourceViewIndices.clear();
	m_samplerTableHandle = {};
}

RhiCpuDescriptorHandle D3D12DescriptorService::GetResourceViewCpuHandle(RhiResourceViewHandle view) const noexcept
{
	const ResourceViewRecord* const record = FindResourceViewRecord(view);
	if (record == nullptr)
	{
		return {};
	}
	return record->copySourceHandle.IsValid() ? RhiCpuDescriptorHandle{record->copySourceHandle.GetCPU().ptr} :
	                                           record->descriptorAllocation.CpuHandle;
}

RhiGpuDescriptorHandle D3D12DescriptorService::GetResourceViewGpuHandle(RhiResourceViewHandle view) const noexcept
{
	const ResourceViewRecord* const record = FindResourceViewRecord(view);
	return record != nullptr ? record->descriptorAllocation.GpuHandle : RhiGpuDescriptorHandle{};
}

NativeTextureViewInfo D3D12DescriptorService::ResolveNativeTextureViewInfo(
	RhiResourceViewHandle,
	RhiResourceHandle resource,
	ResourceState state) const noexcept
{
	return NativeTextureViewInfo{
	    .Resource = NativeResourceHandle{resource.Value},
	    .NativeState = static_cast<std::uint32_t>(D3D12TypeConversions::ToResourceStates(state))};
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
			viewDesc.Format = D3D12DescriptorFormatSelection::ResolveTextureShaderResourceViewFormat(desc.Format);
			viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			if (desc.TextureDimension == TextureResourceDimension::TextureCube)
			{
				viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
				viewDesc.TextureCube.MostDetailedMip = desc.Texture.MostDetailedMip;
				viewDesc.TextureCube.MipLevels = desc.Texture.MipCount;
				viewDesc.TextureCube.ResourceMinLODClamp = 0.0f;
			}
			else
			{
				viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				viewDesc.Texture2D.MostDetailedMip = desc.Texture.MostDetailedMip;
				viewDesc.Texture2D.MipLevels = desc.Texture.MipCount;
				viewDesc.Texture2D.ResourceMinLODClamp = 0.0f;
			}
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
	std::uint32_t recordIndex = 0;
	std::uint16_t generation = 0;
	if (!tableHandle.Decode(recordIndex, generation) || recordIndex >= m_descriptorTableRecords.size())
	{
		return nullptr;
	}

	DescriptorTableRecord& record = m_descriptorTableRecords[recordIndex];
	return record.IsAllocated() && record.generation == generation ? &record : nullptr;
}

const D3D12DescriptorService::DescriptorTableRecord* D3D12DescriptorService::FindDescriptorTableRecord(
    RhiDescriptorTableHandle tableHandle) const noexcept
{
	std::uint32_t recordIndex = 0;
	std::uint16_t generation = 0;
	if (!tableHandle.Decode(recordIndex, generation) || recordIndex >= m_descriptorTableRecords.size())
	{
		return nullptr;
	}

	const DescriptorTableRecord& record = m_descriptorTableRecords[recordIndex];
	return record.IsAllocated() && record.generation == generation ? &record : nullptr;
}

D3D12DescriptorService::ResourceViewRecord* D3D12DescriptorService::FindResourceViewRecord(RhiResourceViewHandle view) noexcept
{
	std::uint32_t recordIndex = 0;
	std::uint16_t generation = 0;
	if (!view.Decode(recordIndex, generation) || recordIndex >= m_resourceViewRecords.size())
	{
		return nullptr;
	}

	ResourceViewRecord& record = m_resourceViewRecords[recordIndex];
	return record.IsAllocated() && record.generation == generation ? &record : nullptr;
}

const D3D12DescriptorService::ResourceViewRecord* D3D12DescriptorService::FindResourceViewRecord(
    RhiResourceViewHandle view) const noexcept
{
	std::uint32_t recordIndex = 0;
	std::uint16_t generation = 0;
	if (!view.Decode(recordIndex, generation) || recordIndex >= m_resourceViewRecords.size())
	{
		return nullptr;
	}

	const ResourceViewRecord& record = m_resourceViewRecords[recordIndex];
	return record.IsAllocated() && record.generation == generation ? &record : nullptr;
}
