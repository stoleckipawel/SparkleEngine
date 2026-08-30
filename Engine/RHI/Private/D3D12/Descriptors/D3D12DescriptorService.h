#pragma once

#include "D3D12/Descriptors/D3D12DescriptorHandle.h"
#include "Descriptors/RhiDescriptorService.h"
#include "Frame/RhiFrameConstants.h"
#include "Interop/RhiNativeHandles.h"

#include <array>
#include <cstdint>
#include <vector>

class D3D12DescriptorHeapManager;
class D3D12InteropService;
class D3D12Rhi;
struct RhiCapabilities;
struct ID3D12DescriptorHeap;

class D3D12DescriptorService final : public RhiDescriptorService
{
public:
	D3D12DescriptorService(D3D12Rhi& rhi, D3D12DescriptorHeapManager& descriptorHeapManager, const RhiCapabilities& capabilities) noexcept;
	~D3D12DescriptorService() noexcept override;

	ID3D12DescriptorHeap* GetShaderResourceDescriptorHeap() const noexcept;
	void BeginFrame(std::uint32_t frameIndex) noexcept override;
	std::unique_ptr<RenderBindingSet> CreateBindingSet(const RenderBindingSetDesc& desc) override;
	void BindGlobalDescriptorState(RenderCommandList& commandList) const noexcept override;
	RhiDescriptorAllocation AllocateDescriptor(ERhiDescriptorAllocatorType descriptorType) override;
	void ReleaseDescriptor(ERhiDescriptorAllocatorType descriptorType, const RhiDescriptorAllocation& allocation) noexcept override;
	RhiDescriptorTableHandle AllocateDescriptorTable(ERhiDescriptorAllocatorType descriptorType, std::uint32_t descriptorCount) override;
	RhiCpuDescriptorHandle GetDescriptorTableCpuHandle(
	    RhiDescriptorTableHandle tableHandle,
	    std::uint32_t descriptorIndex = 0) const noexcept override;
	RhiGpuDescriptorHandle GetDescriptorTableGpuHandle(
	    RhiDescriptorTableHandle tableHandle,
	    std::uint32_t descriptorIndex = 0) const noexcept;
	void ReleaseDescriptorTable(RhiDescriptorTableHandle tableHandle) noexcept override;
	RhiDescriptorTableBinding GetSharedSamplerBinding(const RhiSamplerDesc& samplerDesc) const noexcept override;
	void SetSamplerTableHandle(RhiDescriptorTableHandle samplerTableHandle) noexcept;
	RhiResourceViewHandle CreateResourceView(const RhiResourceViewDesc& desc) override;
	bool WriteResourceView(
	    RhiDescriptorTableHandle tableHandle,
	    std::uint32_t descriptorIndex,
	    RhiResourceViewHandle view) noexcept override;
	void ReleaseResourceView(RhiResourceViewHandle view) noexcept override;
	RhiCpuDescriptorHandle GetResourceViewCpuHandle(RhiResourceViewHandle view) const noexcept override;
	RhiGpuDescriptorHandle GetResourceViewGpuHandle(RhiResourceViewHandle view) const noexcept override;

private:
	friend class D3D12InteropService;

	NativeTextureViewInfo ResolveNativeTextureViewInfo(
	    RhiResourceViewHandle view,
	    RhiResourceHandle resource,
	    ResourceState state) const noexcept;

	struct DescriptorTableRecord
	{
		ERhiDescriptorAllocatorType descriptorType = ERhiDescriptorAllocatorType::ShaderResource;
		std::uint32_t descriptorCount = 0;
		D3D12DescriptorHandle nativeHandle;
		std::uint16_t generation = 0;

		bool IsAllocated() const noexcept { return nativeHandle.IsValid(); }
	};

	struct RetiredDescriptorTable
	{
		ERhiDescriptorAllocatorType descriptorType = ERhiDescriptorAllocatorType::ShaderResource;
		std::uint32_t descriptorCount = 0;
		D3D12DescriptorHandle nativeHandle;
		std::uint32_t recordIndex = 0;
	};

	struct ResourceViewRecord
	{
		ERhiResourceViewKind kind = ERhiResourceViewKind::TextureShaderResource;
		ERhiDescriptorAllocatorType descriptorType = ERhiDescriptorAllocatorType::ShaderResource;
		RhiDescriptorAllocation descriptorAllocation = {};
		D3D12DescriptorHandle copySourceHandle = {};
		std::uint16_t generation = 0;

		bool IsAllocated() const noexcept { return descriptorAllocation.IsValid(); }
	};

	struct RetiredResourceView
	{
		ResourceViewRecord record;
		std::uint32_t recordIndex = 0;
	};

	struct RetiredDescriptorAllocation
	{
		ERhiDescriptorAllocatorType descriptorType = ERhiDescriptorAllocatorType::ShaderResource;
		RhiDescriptorAllocation allocation = {};
	};

	static ERhiDescriptorAllocatorType ResolveResourceViewDescriptorAllocatorType(ERhiResourceViewKind kind) noexcept;
	bool WriteResourceViewDescriptor(const RhiResourceViewDesc& desc, RhiCpuDescriptorHandle destination) noexcept;
	DescriptorTableRecord* FindDescriptorTableRecord(RhiDescriptorTableHandle tableHandle) noexcept;
	const DescriptorTableRecord* FindDescriptorTableRecord(RhiDescriptorTableHandle tableHandle) const noexcept;
	ResourceViewRecord* FindResourceViewRecord(RhiResourceViewHandle view) noexcept;
	const ResourceViewRecord* FindResourceViewRecord(RhiResourceViewHandle view) const noexcept;
	void DestroyDescriptorAllocation(ERhiDescriptorAllocatorType descriptorType, const RhiDescriptorAllocation& allocation) noexcept;
	void DestroyDescriptorTable(
	    ERhiDescriptorAllocatorType descriptorType,
	    const D3D12DescriptorHandle& nativeHandle,
	    std::uint32_t descriptorCount) noexcept;
	void RecycleDescriptorTableRecord(std::uint32_t recordIndex) noexcept;
	void DestroyResourceView(ResourceViewRecord& record) noexcept;
	void RecycleResourceViewRecord(std::uint32_t recordIndex) noexcept;
	void ReleaseAllDescriptors() noexcept;

	D3D12Rhi* m_rhi = nullptr;
	D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;
	const RhiCapabilities* m_capabilities = nullptr;
	RhiDescriptorTableHandle m_samplerTableHandle = {};
	std::vector<DescriptorTableRecord> m_descriptorTableRecords;
	std::vector<std::uint32_t> m_freeDescriptorTableIndices;
	std::vector<ResourceViewRecord> m_resourceViewRecords;
	std::vector<std::uint32_t> m_freeResourceViewIndices;
	std::array<std::vector<RetiredDescriptorAllocation>, RhiFrameConstants::MaxFrameSlotCount> m_retiredDescriptorAllocations;
	std::array<std::vector<RetiredDescriptorTable>, RhiFrameConstants::MaxFrameSlotCount> m_retiredDescriptorTables;
	std::array<std::vector<RetiredResourceView>, RhiFrameConstants::MaxFrameSlotCount> m_retiredResourceViews;
	std::uint32_t m_currentFrameIndex = 0;
};
