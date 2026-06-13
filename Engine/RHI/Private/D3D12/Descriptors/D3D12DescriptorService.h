#pragma once

#include "D3D12/Descriptors/D3D12DescriptorHandle.h"
#include "Descriptors/RhiDescriptorHandles.h"
#include "Resources/RhiResourceView.h"
#include "Samplers/RhiSamplerDesc.h"

#include <cstdint>
#include <vector>

class D3D12DescriptorHeapManager;
class D3D12Rhi;
struct ID3D12DescriptorHeap;

class D3D12DescriptorService final
{
  public:
	D3D12DescriptorService(D3D12Rhi& rhi, D3D12DescriptorHeapManager& descriptorHeapManager) noexcept;

	ID3D12DescriptorHeap* GetShaderResourceDescriptorHeap() const noexcept;
	RhiDescriptorAllocation AllocateDescriptor(ERhiDescriptorAllocatorType descriptorType);
	void ReleaseDescriptor(ERhiDescriptorAllocatorType descriptorType, const RhiDescriptorAllocation& allocation) noexcept;
	RhiDescriptorTableHandle AllocateDescriptorTable(ERhiDescriptorAllocatorType descriptorType, std::uint32_t descriptorCount);
	RhiCpuDescriptorHandle GetDescriptorTableCpuHandle(
	    RhiDescriptorTableHandle tableHandle,
	    std::uint32_t descriptorIndex = 0) const noexcept;
	RhiGpuDescriptorHandle GetDescriptorTableGpuHandle(
	    RhiDescriptorTableHandle tableHandle,
	    std::uint32_t descriptorIndex = 0) const noexcept;
	void ReleaseDescriptorTable(RhiDescriptorTableHandle tableHandle) noexcept;
	void AllocateShaderResourceDescriptor(RhiCpuDescriptorHandle& outCpuHandle, RhiGpuDescriptorHandle& outGpuHandle);
	void ReleaseShaderResourceDescriptor(RhiCpuDescriptorHandle cpuHandle, RhiGpuDescriptorHandle gpuHandle) noexcept;
	RhiDescriptorTableBinding GetSharedSamplerBinding(const RhiSamplerDesc& samplerDesc) const noexcept;
	void SetSamplerTableHandle(RhiDescriptorTableHandle samplerTableHandle) noexcept;
	RhiResourceViewHandle CreateResourceView(const RhiResourceViewDesc& desc);
	void ReleaseResourceView(RhiResourceViewHandle view) noexcept;
	RhiCpuDescriptorHandle GetResourceViewCpuHandle(RhiResourceViewHandle view) const noexcept;
	RhiGpuDescriptorHandle GetResourceViewGpuHandle(RhiResourceViewHandle view) const noexcept;

  private:
	struct DescriptorTableRecord
	{
		ERhiDescriptorAllocatorType descriptorType = ERhiDescriptorAllocatorType::ShaderResource;
		std::uint32_t descriptorCount = 0;
		D3D12DescriptorHandle nativeHandle;

		bool IsAllocated() const noexcept { return nativeHandle.IsValid(); }
	};

	struct ResourceViewRecord
	{
		ERhiResourceViewKind kind = ERhiResourceViewKind::TextureShaderResource;
		ERhiDescriptorAllocatorType descriptorType = ERhiDescriptorAllocatorType::ShaderResource;
		RhiDescriptorAllocation descriptorAllocation = {};

		bool IsAllocated() const noexcept { return descriptorAllocation.IsValid(); }
	};

	static ERhiDescriptorAllocatorType ResolveResourceViewDescriptorAllocatorType(ERhiResourceViewKind kind) noexcept;
	bool WriteResourceViewDescriptor(const RhiResourceViewDesc& desc, RhiCpuDescriptorHandle destination) noexcept;
	DescriptorTableRecord* FindDescriptorTableRecord(RhiDescriptorTableHandle tableHandle) noexcept;
	const DescriptorTableRecord* FindDescriptorTableRecord(RhiDescriptorTableHandle tableHandle) const noexcept;
	ResourceViewRecord* FindResourceViewRecord(RhiResourceViewHandle view) noexcept;
	const ResourceViewRecord* FindResourceViewRecord(RhiResourceViewHandle view) const noexcept;

	D3D12Rhi* m_rhi = nullptr;
	D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;
	RhiDescriptorTableHandle m_samplerTableHandle = {};
	std::vector<DescriptorTableRecord> m_descriptorTableRecords;
	std::vector<std::uint32_t> m_freeDescriptorTableIndices;
	std::vector<ResourceViewRecord> m_resourceViewRecords;
	std::vector<std::uint32_t> m_freeResourceViewIndices;
};
