#pragma once

#include "Descriptors/RhiDescriptorHandles.h"
#include "Frame/RhiFrameConstants.h"
#include "Resources/RhiResourceView.h"
#include "Samplers/RhiSamplerDesc.h"
#include "Vulkan/VulkanIncludes.h"

#include <atomic>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <vector>

class VulkanRhi;
class VulkanDescriptorService;
struct CompiledBinding;

class VulkanDescriptorAllocator final
{
  public:
	explicit VulkanDescriptorAllocator(VulkanRhi& rhi) noexcept;
	~VulkanDescriptorAllocator() noexcept = default;

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
	void WriteSamplerDescriptor(RhiCpuDescriptorHandle destination, VkSampler sampler) noexcept;
	bool WriteRegisteredDescriptor(RhiCpuDescriptorHandle destination, RhiGpuDescriptorHandle source) noexcept;

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
	void WriteAccelerationStructureDescriptor(
	    VkDescriptorSet descriptorSet,
	    const CompiledBinding& binding,
	    VkAccelerationStructureKHR accelerationStructure) noexcept;
	void WritePartitionedAccelerationStructureDescriptor(
	    VkDescriptorSet descriptorSet,
	    const CompiledBinding& binding,
	    VkDeviceAddress accelerationStructureAddress) noexcept;

  private:
	friend class VulkanDescriptorService;

	enum class EntryKind : std::uint8_t
	{
		Empty,
		SampledImage,
		StorageImage,
		UniformBuffer,
		StorageBuffer,
		Sampler,
		AccelerationStructure,
		PartitionedAccelerationStructure,
	};
	struct DescriptorEntry final
	{
		EntryKind Kind = EntryKind::Empty;
		VkDescriptorImageInfo Image = {};
		VkDescriptorBufferInfo Buffer = {};
		VkAccelerationStructureKHR AccelerationStructure = VK_NULL_HANDLE;
		VkDeviceAddress PartitionedAccelerationStructureAddress = 0;
	};

	static constexpr std::size_t DescriptorWriteChunkCapacity = 64;

	struct DescriptorWriteChunk final
	{
		std::array<VkDescriptorImageInfo, DescriptorWriteChunkCapacity> ImageInfos;
		std::array<VkDescriptorBufferInfo, DescriptorWriteChunkCapacity> BufferInfos;
		std::array<VkAccelerationStructureKHR, DescriptorWriteChunkCapacity> AccelerationStructures;
		std::array<VkDeviceAddress, DescriptorWriteChunkCapacity> PartitionedAccelerationStructureAddresses;
		std::uint32_t Count = 0;
	};

	struct DescriptorTableRecord final
	{
		ERhiDescriptorAllocatorType Type = ERhiDescriptorAllocatorType::ShaderResource;
		std::shared_ptr<std::vector<DescriptorEntry>> Entries;
		bool Allocated = false;
		std::uint16_t Generation = 0;
	};

	struct DescriptorTableReadRecord final
	{
		std::shared_ptr<const std::vector<DescriptorEntry>> Entries;
		bool Allocated = false;
		std::uint16_t Generation = 0;
	};

	struct RecordingReadView final
	{
		std::vector<DescriptorTableReadRecord> Tables;
		std::shared_ptr<const std::vector<DescriptorEntry>> RegisteredDescriptors;
	};

	static VkDescriptorType ToDescriptorType(EntryKind kind) noexcept;
	static EntryKind ToImageEntryKind(ERhiResourceViewKind viewKind) noexcept;
	static EntryKind ToBufferEntryKind(ERhiResourceViewKind viewKind) noexcept;
	static VkImageLayout ToImageLayout(EntryKind kind) noexcept;

	DescriptorTableRecord* FindTableRecord(RhiDescriptorTableHandle tableHandle) noexcept;
	const DescriptorTableRecord* FindTableRecord(RhiDescriptorTableHandle tableHandle) const noexcept;
	std::vector<DescriptorEntry>& EditTableEntries(DescriptorTableRecord& record);
	std::vector<DescriptorEntry>& EditRegisteredDescriptors();
	void PublishRecordingReadView() noexcept;
	void RecycleTableRecord(std::uint32_t tableIndex) noexcept;
	DescriptorEntry* FindRegisteredEntry(RhiGpuDescriptorHandle handle) noexcept;
	const DescriptorEntry* FindRegisteredEntry(RhiGpuDescriptorHandle handle) const noexcept;
	std::shared_ptr<const RecordingReadView> GetRecordingReadView() const noexcept;
	static bool EntryKindMatchesBinding(const CompiledBinding& binding, EntryKind entryKind) noexcept;
	static bool BuildWriteChunk(std::span<const DescriptorEntry> entries, EntryKind entryKind, DescriptorWriteChunk& outChunk) noexcept;
	void CommitWriteChunk(
	    VkDescriptorSet descriptorSet,
	    const CompiledBinding& binding,
	    EntryKind entryKind,
	    VkDescriptorType descriptorType,
	    std::uint32_t firstDescriptor,
	    const DescriptorWriteChunk& chunk) noexcept;
	void WriteEntries(VkDescriptorSet descriptorSet, const CompiledBinding& binding, std::span<const DescriptorEntry> entries) noexcept;

	VulkanRhi& m_rhi;
	mutable std::mutex m_registryMutex;
	std::uint32_t m_currentFrameIndex = 0;
	std::vector<DescriptorTableRecord> m_tables;
	std::vector<std::uint32_t> m_freeTableIndices;
	std::array<std::vector<std::uint32_t>, RhiFrameConstants::MaxFrameSlotCount> m_retiredTableIndices;
	std::shared_ptr<std::vector<DescriptorEntry>> m_registeredDescriptors;
	std::atomic<std::shared_ptr<const RecordingReadView>> m_recordingReadView;
	std::vector<std::uint32_t> m_freeRegisteredDescriptorIndices;
};
