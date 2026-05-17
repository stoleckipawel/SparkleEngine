#pragma once

#include "Descriptors/RhiDescriptorHandles.h"
#include "Resources/RhiResourceView.h"
#include "Samplers/RhiSamplerDesc.h"
#include "Vulkan/VulkanIncludes.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <vector>

class VulkanRhi;
struct CompiledBinding;

class VulkanDescriptorAllocator final
{
  public:
	explicit VulkanDescriptorAllocator(VulkanRhi& rhi) noexcept;
	~VulkanDescriptorAllocator() noexcept;

	VulkanDescriptorAllocator(const VulkanDescriptorAllocator&) = delete;
	VulkanDescriptorAllocator& operator=(const VulkanDescriptorAllocator&) = delete;
	VulkanDescriptorAllocator(VulkanDescriptorAllocator&&) = delete;
	VulkanDescriptorAllocator& operator=(VulkanDescriptorAllocator&&) = delete;

	RhiDescriptorAllocation AllocateDescriptor(ERhiDescriptorAllocatorType descriptorType);
	void BeginFrame(std::uint32_t frameIndex) noexcept;
	void ReleaseDescriptor(ERhiDescriptorAllocatorType descriptorType, const RhiDescriptorAllocation& allocation) noexcept;
	RhiDescriptorTableHandle AllocateDescriptorTable(ERhiDescriptorAllocatorType descriptorType, std::uint32_t descriptorCount);
	RhiCpuDescriptorHandle GetDescriptorTableCpuHandle(RhiDescriptorTableHandle tableHandle, std::uint32_t descriptorIndex = 0)
	    const noexcept;
	void ReleaseDescriptorTable(RhiDescriptorTableHandle tableHandle) noexcept;

	RhiGpuDescriptorHandle RegisterImageDescriptor(ERhiResourceViewKind viewKind, VkImageView imageView);
	RhiGpuDescriptorHandle RegisterBufferDescriptor(
	    ERhiResourceViewKind viewKind,
	    VkBuffer buffer,
	    std::uint64_t offsetInBytes,
	    std::uint64_t sizeInBytes);
	void ReleaseRegisteredDescriptor(RhiGpuDescriptorHandle handle) noexcept;
	void WriteImageDescriptor(RhiCpuDescriptorHandle destination, ERhiResourceViewKind viewKind, VkImageView imageView) noexcept;
	void WriteSamplerDescriptor(RhiCpuDescriptorHandle destination, VkSampler sampler) noexcept;

	VkDescriptorSet AllocateTransientSet(VkDescriptorSetLayout layout);
	void WriteDescriptorTable(
	    VkDescriptorSet descriptorSet,
	    const CompiledBinding& binding,
	    RhiDescriptorTableBinding tableBinding) noexcept;
	void WriteDescriptorHandle(VkDescriptorSet descriptorSet, const CompiledBinding& binding, RhiGpuDescriptorHandle handle) noexcept;
	void WriteBufferDescriptor(
	    VkDescriptorSet descriptorSet,
	    const CompiledBinding& binding,
	    VkBuffer buffer,
	    VkDeviceSize offset,
	    VkDeviceSize range) noexcept;

  private:
	enum class EntryKind : std::uint8_t
	{
		Empty,
		SampledImage,
		StorageImage,
		UniformBuffer,
		StorageBuffer,
		Sampler,
	};

	struct DescriptorEntry final
	{
		EntryKind Kind = EntryKind::Empty;
		VkDescriptorImageInfo Image = {};
		VkDescriptorBufferInfo Buffer = {};
	};

	struct DescriptorTableRecord final
	{
		ERhiDescriptorAllocatorType Type = ERhiDescriptorAllocatorType::ShaderResource;
		std::vector<DescriptorEntry> Entries;
		bool Allocated = false;
	};

	struct DescriptorPoolPage final
	{
		VkDescriptorPool Pool = VK_NULL_HANDLE;
		std::uint32_t AllocatedSets = 0;
	};

	static constexpr std::uint32_t DescriptorSetsPerPage = 256;

	static VkDescriptorType ToDescriptorType(EntryKind kind) noexcept;
	static EntryKind ToImageEntryKind(ERhiResourceViewKind viewKind) noexcept;
	static EntryKind ToBufferEntryKind(ERhiResourceViewKind viewKind) noexcept;
	static EntryKind ToTableEntryKind(ERhiDescriptorAllocatorType descriptorType) noexcept;
	static VkImageLayout ToImageLayout(EntryKind kind) noexcept;

	DescriptorTableRecord* FindTableRecord(RhiDescriptorTableHandle tableHandle) noexcept;
	const DescriptorTableRecord* FindTableRecord(RhiDescriptorTableHandle tableHandle) const noexcept;
	DescriptorEntry* FindRegisteredEntry(RhiGpuDescriptorHandle handle) noexcept;
	const DescriptorEntry* FindRegisteredEntry(RhiGpuDescriptorHandle handle) const noexcept;
	VkDescriptorPool CreatePoolPage();
	void WriteEntries(VkDescriptorSet descriptorSet, const CompiledBinding& binding, std::span<const DescriptorEntry> entries) noexcept;

	VulkanRhi& m_rhi;
	mutable std::mutex m_mutex;
	std::uint32_t m_currentFrameIndex = 0;
	std::vector<std::vector<DescriptorPoolPage>> m_framePoolPages;
	std::vector<DescriptorTableRecord> m_tables;
	std::vector<std::uint32_t> m_freeTableIndices;
	std::vector<DescriptorEntry> m_registeredDescriptors;
	std::vector<std::uint32_t> m_freeRegisteredDescriptorIndices;
};