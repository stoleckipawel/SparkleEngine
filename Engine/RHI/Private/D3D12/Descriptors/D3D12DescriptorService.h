#pragma once

#include "D3D12/Descriptors/D3D12DescriptorHandle.h"
#include "Descriptors/RhiDescriptorService.h"
#include "Frame/RhiFrameConstants.h"

#include <array>
#include <cstdint>
#include <vector>

class D3D12DescriptorHeapManager;
class D3D12Rhi;
struct RhiCapabilities;
struct ID3D12DescriptorHeap;

class D3D12DescriptorService final : public RhiDescriptorService
{
  public:
	D3D12DescriptorService(
	    D3D12Rhi& rhi,
	    D3D12DescriptorHeapManager& descriptorHeapManager,
	    const RhiCapabilities& capabilities) noexcept;
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
	void AllocateShaderResourceDescriptor(RhiCpuDescriptorHandle& outCpuHandle, RhiGpuDescriptorHandle& outGpuHandle) override;
	void ReleaseShaderResourceDescriptor(RhiCpuDescriptorHandle cpuHandle, RhiGpuDescriptorHandle gpuHandle) noexcept override;
	RhiDescriptorTableBinding GetSharedSamplerBinding(const RhiSamplerDesc& samplerDesc) const noexcept override;
	void SetSamplerTableHandle(RhiDescriptorTableHandle samplerTableHandle) noexcept;
	RhiResourceViewHandle CreateResourceView(const RhiResourceViewDesc& desc) override;
	void ReleaseResourceView(RhiResourceViewHandle view) noexcept override;
	RhiCpuDescriptorHandle GetResourceViewCpuHandle(RhiResourceViewHandle view) const noexcept override;
	RhiGpuDescriptorHandle GetResourceViewGpuHandle(RhiResourceViewHandle view) const noexcept override;
	NativeTextureViewInfo GetNativeTextureViewInfo(RhiResourceViewHandle view, ResourceState state) const noexcept override;

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
	void DestroyResourceView(ResourceViewRecord& record) noexcept;
	void ReleaseAllResourceViews() noexcept;

	D3D12Rhi* m_rhi = nullptr;
	D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;
	const RhiCapabilities* m_capabilities = nullptr;
	RhiDescriptorTableHandle m_samplerTableHandle = {};
	std::vector<DescriptorTableRecord> m_descriptorTableRecords;
	std::vector<std::uint32_t> m_freeDescriptorTableIndices;
	std::vector<ResourceViewRecord> m_resourceViewRecords;
	std::vector<std::uint32_t> m_freeResourceViewIndices;
	std::array<std::vector<ResourceViewRecord>, RhiFrameConstants::FramesInFlight> m_retiredResourceViews;
	std::uint32_t m_currentFrameIndex = 0;
};
