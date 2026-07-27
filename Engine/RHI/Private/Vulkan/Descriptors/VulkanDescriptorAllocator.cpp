#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Descriptors/VulkanDescriptorAllocator.h"

#include "Pipeline/RhiPipelineStateDesc.h"
#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Descriptors/VulkanDescriptorHandles.h"
#include "Vulkan/Device/VulkanRhi.h"

#include <algorithm>
#include <array>

VulkanDescriptorAllocator::VulkanDescriptorAllocator(VulkanRhi& rhi) noexcept :
	m_rhi(rhi),
	m_registeredDescriptors(
	    std::make_shared<std::vector<DescriptorEntry>>())
{
	CreateFallbackBuffer();
	PublishRecordingReadView();
}

VulkanDescriptorAllocator::~VulkanDescriptorAllocator() noexcept
{
	if (m_fallbackBuffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(m_rhi.GetDevice(), m_fallbackBuffer, nullptr);
		m_fallbackBuffer = VK_NULL_HANDLE;
	}
	if (m_fallbackBufferMemory != VK_NULL_HANDLE)
	{
		vkFreeMemory(m_rhi.GetDevice(), m_fallbackBufferMemory, nullptr);
		m_fallbackBufferMemory = VK_NULL_HANDLE;
	}
}

RhiDescriptorAllocation VulkanDescriptorAllocator::AllocateDescriptor(ERhiDescriptorAllocatorType descriptorType)
{
	const RhiDescriptorTableHandle table = AllocateDescriptorTable(descriptorType, 1);
	return RhiDescriptorAllocation{.CpuHandle = GetDescriptorTableCpuHandle(table), .GpuHandle = {}};
}

void VulkanDescriptorAllocator::BeginFrame(std::uint32_t frameIndex) noexcept
{
	if (frameIndex >= RhiFrameConstants::FramesInFlight)
	{
		return;
	}

	{
		std::scoped_lock lock(m_registryMutex);
		for (const std::uint32_t tableIndex :
		     m_retiredTableIndices[frameIndex])
		{
			RecycleTableRecord(tableIndex);
		}
		m_retiredTableIndices[frameIndex].clear();
		m_currentFrameIndex = frameIndex;
	}
}

void VulkanDescriptorAllocator::PublishRecordingReadView() noexcept
{
	auto readView = std::make_shared<RecordingReadView>();
	{
		std::scoped_lock lock(m_registryMutex);
		readView->Tables.reserve(m_tables.size());
		for (const DescriptorTableRecord& table : m_tables)
		{
			readView->Tables.push_back(
			    DescriptorTableReadRecord{
			        .Entries = table.Entries,
			        .Allocated = table.Allocated,
			        .Generation = table.Generation});
		}
		readView->RegisteredDescriptors = m_registeredDescriptors;
	}

	m_recordingReadView.store(
	    std::shared_ptr<const RecordingReadView>(std::move(readView)),
	    std::memory_order_release);
}

void VulkanDescriptorAllocator::ReleaseDescriptor(ERhiDescriptorAllocatorType, const RhiDescriptorAllocation& allocation) noexcept
{
	RhiDescriptorTableHandle tableHandle{};
	std::uint32_t descriptorIndex = 0;
	if (!VulkanDescriptorHandles::DecodeCpuDescriptorHandle(allocation.CpuHandle, tableHandle, descriptorIndex) || descriptorIndex != 0)
	{
		return;
	}
	ReleaseDescriptorTable(tableHandle);
}

RhiDescriptorTableHandle VulkanDescriptorAllocator::AllocateDescriptorTable(
    ERhiDescriptorAllocatorType descriptorType,
    std::uint32_t descriptorCount)
{
	if (descriptorCount == 0 || descriptorCount > VulkanDescriptorHandles::MaximumCpuDescriptorCount)
	{
		return {};
	}

	std::scoped_lock lock(m_registryMutex);
	if (m_freeTableIndices.empty() && m_tables.size() >= RhiDescriptorTableHandle::MaximumRecordCount)
	{
		return {};
	}

	DescriptorTableRecord record{};
	record.Type = descriptorType;
	record.Entries =
	    std::make_shared<std::vector<DescriptorEntry>>(
	        descriptorCount,
	        DescriptorEntry{});
	record.Allocated = true;

	if (!m_freeTableIndices.empty())
	{
		const std::uint32_t index = m_freeTableIndices.back();
		m_freeTableIndices.pop_back();
		record.Generation = m_tables[index].Generation;
		m_tables[index] = std::move(record);
		return VulkanDescriptorHandles::MakeTableHandle(index, m_tables[index].Generation);
	}

	m_tables.push_back(std::move(record));
	return VulkanDescriptorHandles::MakeTableHandle(static_cast<std::uint32_t>(m_tables.size() - 1), 0u);
}

RhiCpuDescriptorHandle VulkanDescriptorAllocator::GetDescriptorTableCpuHandle(
    RhiDescriptorTableHandle tableHandle,
    std::uint32_t descriptorIndex) const noexcept
{
	std::scoped_lock lock(m_registryMutex);
	const DescriptorTableRecord* const record = FindTableRecord(tableHandle);
	if (record == nullptr ||
	    record->Entries == nullptr ||
	    descriptorIndex >= record->Entries->size())
	{
		return {};
	}
	return VulkanDescriptorHandles::MakeCpuDescriptorHandle(tableHandle, descriptorIndex);
}

void VulkanDescriptorAllocator::ReleaseDescriptorTable(RhiDescriptorTableHandle tableHandle) noexcept
{
	std::scoped_lock lock(m_registryMutex);
	DescriptorTableRecord* const record = FindTableRecord(tableHandle);
	if (record == nullptr)
	{
		return;
	}

	std::uint32_t tableIndex = 0;
	std::uint16_t generation = 0;
	if (!tableHandle.Decode(tableIndex, generation))
	{
		return;
	}

	record->Entries.reset();
	record->Allocated = false;
	m_retiredTableIndices[m_currentFrameIndex].push_back(tableIndex);
}

RhiGpuDescriptorHandle VulkanDescriptorAllocator::RegisterImageDescriptor(ERhiResourceViewKind viewKind, VkImageView imageView)
{
	if (imageView == VK_NULL_HANDLE)
	{
		return {};
	}

	DescriptorEntry entry{};
	entry.Kind = ToImageEntryKind(viewKind);
	entry.Image = VkDescriptorImageInfo{.sampler = VK_NULL_HANDLE, .imageView = imageView, .imageLayout = ToImageLayout(entry.Kind)};

	std::scoped_lock lock(m_registryMutex);
	std::vector<DescriptorEntry>& descriptors = EditRegisteredDescriptors();
	if (!m_freeRegisteredDescriptorIndices.empty())
	{
		const std::uint32_t index = m_freeRegisteredDescriptorIndices.back();
		m_freeRegisteredDescriptorIndices.pop_back();
		descriptors[index] = entry;
		return VulkanDescriptorHandles::MakeGpuDescriptorHandle(index);
	}

	descriptors.push_back(entry);
	return VulkanDescriptorHandles::MakeGpuDescriptorHandle(
	    static_cast<std::uint32_t>(descriptors.size() - 1));
}

RhiGpuDescriptorHandle VulkanDescriptorAllocator::RegisterBufferDescriptor(
    ERhiResourceViewKind viewKind,
    VkBuffer buffer,
    std::uint64_t offsetInBytes,
    std::uint64_t sizeInBytes)
{
	if (buffer == VK_NULL_HANDLE)
	{
		return {};
	}

	DescriptorEntry entry{};
	entry.Kind = ToBufferEntryKind(viewKind);
	entry.Buffer =
	    VkDescriptorBufferInfo{.buffer = buffer, .offset = offsetInBytes, .range = sizeInBytes != 0 ? sizeInBytes : VK_WHOLE_SIZE};

	std::scoped_lock lock(m_registryMutex);
	std::vector<DescriptorEntry>& descriptors = EditRegisteredDescriptors();
	if (!m_freeRegisteredDescriptorIndices.empty())
	{
		const std::uint32_t index = m_freeRegisteredDescriptorIndices.back();
		m_freeRegisteredDescriptorIndices.pop_back();
		descriptors[index] = entry;
		return VulkanDescriptorHandles::MakeGpuDescriptorHandle(index);
	}

	descriptors.push_back(entry);
	return VulkanDescriptorHandles::MakeGpuDescriptorHandle(
	    static_cast<std::uint32_t>(descriptors.size() - 1));
}

RhiGpuDescriptorHandle VulkanDescriptorAllocator::RegisterAccelerationStructureDescriptor(VkAccelerationStructureKHR accelerationStructure)
{
	if (accelerationStructure == VK_NULL_HANDLE)
	{
		return {};
	}

	DescriptorEntry entry{};
	entry.Kind = EntryKind::AccelerationStructure;
	entry.AccelerationStructure = accelerationStructure;

	std::scoped_lock lock(m_registryMutex);
	std::vector<DescriptorEntry>& descriptors = EditRegisteredDescriptors();
	if (!m_freeRegisteredDescriptorIndices.empty())
	{
		const std::uint32_t index = m_freeRegisteredDescriptorIndices.back();
		m_freeRegisteredDescriptorIndices.pop_back();
		descriptors[index] = entry;
		return VulkanDescriptorHandles::MakeGpuDescriptorHandle(index);
	}

	descriptors.push_back(entry);
	return VulkanDescriptorHandles::MakeGpuDescriptorHandle(
	    static_cast<std::uint32_t>(descriptors.size() - 1));
}

RhiGpuDescriptorHandle VulkanDescriptorAllocator::RegisterPartitionedAccelerationStructureDescriptor(
    VkDeviceAddress accelerationStructureAddress)
{
	if (accelerationStructureAddress == 0)
	{
		return {};
	}

	DescriptorEntry entry{};
	entry.Kind = EntryKind::PartitionedAccelerationStructure;
	entry.PartitionedAccelerationStructureAddress = accelerationStructureAddress;

	std::scoped_lock lock(m_registryMutex);
	std::vector<DescriptorEntry>& descriptors = EditRegisteredDescriptors();
	if (!m_freeRegisteredDescriptorIndices.empty())
	{
		const std::uint32_t index = m_freeRegisteredDescriptorIndices.back();
		m_freeRegisteredDescriptorIndices.pop_back();
		descriptors[index] = entry;
		return VulkanDescriptorHandles::MakeGpuDescriptorHandle(index);
	}

	descriptors.push_back(entry);
	return VulkanDescriptorHandles::MakeGpuDescriptorHandle(
	    static_cast<std::uint32_t>(descriptors.size() - 1));
}

void VulkanDescriptorAllocator::ReleaseRegisteredDescriptor(RhiGpuDescriptorHandle handle) noexcept
{
	std::uint32_t index = 0;
	if (!VulkanDescriptorHandles::DecodeGpuDescriptorHandle(handle, index))
	{
		return;
	}

	std::scoped_lock lock(m_registryMutex);
	std::vector<DescriptorEntry>& descriptors = EditRegisteredDescriptors();
	if (index >= descriptors.size())
	{
		return;
	}
	if (descriptors[index].Kind == EntryKind::Empty)
	{
		return;
	}
	descriptors[index] = DescriptorEntry{};
	m_freeRegisteredDescriptorIndices.push_back(index);
}

void VulkanDescriptorAllocator::WriteImageDescriptor(
    RhiCpuDescriptorHandle destination,
    ERhiResourceViewKind viewKind,
    VkImageView imageView) noexcept
{
	RhiDescriptorTableHandle tableHandle{};
	std::uint32_t descriptorIndex = 0;
	if (!VulkanDescriptorHandles::DecodeCpuDescriptorHandle(destination, tableHandle, descriptorIndex) || imageView == VK_NULL_HANDLE)
	{
		return;
	}

	std::scoped_lock lock(m_registryMutex);
	DescriptorTableRecord* const record = FindTableRecord(tableHandle);
	if (record == nullptr ||
	    record->Entries == nullptr ||
	    descriptorIndex >= record->Entries->size())
	{
		return;
	}
	std::vector<DescriptorEntry>& entries = EditTableEntries(*record);
	DescriptorEntry& entry = entries[descriptorIndex];
	entry.Kind = ToImageEntryKind(viewKind);
	entry.Image = VkDescriptorImageInfo{.sampler = VK_NULL_HANDLE, .imageView = imageView, .imageLayout = ToImageLayout(entry.Kind)};
}

void VulkanDescriptorAllocator::WriteSamplerDescriptor(RhiCpuDescriptorHandle destination, VkSampler sampler) noexcept
{
	RhiDescriptorTableHandle tableHandle{};
	std::uint32_t descriptorIndex = 0;
	if (!VulkanDescriptorHandles::DecodeCpuDescriptorHandle(destination, tableHandle, descriptorIndex) || sampler == VK_NULL_HANDLE)
	{
		return;
	}

	std::scoped_lock lock(m_registryMutex);
	DescriptorTableRecord* const record = FindTableRecord(tableHandle);
	if (record == nullptr ||
	    record->Entries == nullptr ||
	    descriptorIndex >= record->Entries->size())
	{
		return;
	}
	std::vector<DescriptorEntry>& entries = EditTableEntries(*record);
	DescriptorEntry& entry = entries[descriptorIndex];
	entry.Kind = EntryKind::Sampler;
	entry.Image = VkDescriptorImageInfo{.sampler = sampler, .imageView = VK_NULL_HANDLE, .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED};
}

bool VulkanDescriptorAllocator::WriteRegisteredDescriptor(
    RhiCpuDescriptorHandle destination,
    RhiGpuDescriptorHandle source) noexcept
{
	RhiDescriptorTableHandle tableHandle{};
	std::uint32_t descriptorIndex = 0;
	if (!VulkanDescriptorHandles::DecodeCpuDescriptorHandle(destination, tableHandle, descriptorIndex))
	{
		return false;
	}

	std::scoped_lock lock(m_registryMutex);
	DescriptorTableRecord* const table = FindTableRecord(tableHandle);
	const DescriptorEntry* const sourceEntry = FindRegisteredEntry(source);
	if (table == nullptr ||
	    table->Entries == nullptr ||
	    sourceEntry == nullptr ||
	    sourceEntry->Kind == EntryKind::Empty ||
	    descriptorIndex >= table->Entries->size())
	{
		return false;
	}

	EditTableEntries(*table)[descriptorIndex] = *sourceEntry;
	return true;
}

void VulkanDescriptorAllocator::WriteDescriptorTable(
    VkDescriptorSet descriptorSet,
    const CompiledBinding& binding,
    RhiDescriptorTableBinding tableBinding) noexcept
{
	if (descriptorSet == VK_NULL_HANDLE || !tableBinding)
	{
		return;
	}

	const std::shared_ptr<const RecordingReadView> readView =
	    GetRecordingReadView();
	std::uint32_t tableIndex = 0;
	std::uint16_t generation = 0;
	if (readView == nullptr ||
	    !tableBinding.Table.Decode(tableIndex, generation) ||
	    tableIndex >= readView->Tables.size())
	{
		return;
	}

	const DescriptorTableReadRecord& table = readView->Tables[tableIndex];
	if (!table.Allocated ||
	    table.Generation != generation ||
	    table.Entries == nullptr ||
	    tableBinding.DescriptorIndex >= table.Entries->size())
	{
		return;
	}

	const std::uint32_t count =
	    std::min(
	        binding.DescriptorCount,
	        static_cast<std::uint32_t>(
	            table.Entries->size() - tableBinding.DescriptorIndex));
	WriteEntries(
	    descriptorSet,
	    binding,
	    std::span<const DescriptorEntry>(
	        table.Entries->data() + tableBinding.DescriptorIndex,
	        count));
}

void VulkanDescriptorAllocator::WriteDescriptorHandle(
    VkDescriptorSet descriptorSet,
    const CompiledBinding& binding,
    RhiGpuDescriptorHandle handle) noexcept
{
	if (descriptorSet == VK_NULL_HANDLE || !handle)
	{
		return;
	}

	const std::shared_ptr<const RecordingReadView> readView =
	    GetRecordingReadView();
	std::uint32_t descriptorIndex = 0;
	if (readView == nullptr ||
	    readView->RegisteredDescriptors == nullptr ||
	    !VulkanDescriptorHandles::DecodeGpuDescriptorHandle(
	        handle,
	        descriptorIndex) ||
	    descriptorIndex >= readView->RegisteredDescriptors->size())
	{
		return;
	}

	const DescriptorEntry& entry =
	    (*readView->RegisteredDescriptors)[descriptorIndex];
	if (entry.Kind == EntryKind::Empty)
	{
		return;
	}

	WriteEntries(
	    descriptorSet,
	    binding,
	    std::span<const DescriptorEntry>(&entry, 1));
}

void VulkanDescriptorAllocator::WriteBufferDescriptor(
    VkDescriptorSet descriptorSet,
    const CompiledBinding& binding,
    VkBuffer buffer,
    VkDeviceSize offset,
    VkDeviceSize range) noexcept
{
	if (descriptorSet == VK_NULL_HANDLE || buffer == VK_NULL_HANDLE)
	{
		return;
	}

	DescriptorEntry entry{};
	entry.Kind = binding.Type == CompiledBindingType::ConstantBuffer ? EntryKind::UniformBuffer : EntryKind::StorageBuffer;
	entry.Buffer = VkDescriptorBufferInfo{.buffer = buffer, .offset = offset, .range = range != 0 ? range : VK_WHOLE_SIZE};
	WriteEntries(descriptorSet, binding, std::span<const DescriptorEntry>(&entry, 1));
}

void VulkanDescriptorAllocator::WriteAccelerationStructureDescriptor(
    VkDescriptorSet descriptorSet,
    const CompiledBinding& binding,
    VkAccelerationStructureKHR accelerationStructure) noexcept
{
	if (descriptorSet == VK_NULL_HANDLE || accelerationStructure == VK_NULL_HANDLE)
	{
		return;
	}

	DescriptorEntry entry{};
	entry.Kind = EntryKind::AccelerationStructure;
	entry.AccelerationStructure = accelerationStructure;
	WriteEntries(descriptorSet, binding, std::span<const DescriptorEntry>(&entry, 1));
}

void VulkanDescriptorAllocator::WritePartitionedAccelerationStructureDescriptor(
    VkDescriptorSet descriptorSet,
    const CompiledBinding& binding,
    VkDeviceAddress accelerationStructureAddress) noexcept
{
	if (descriptorSet == VK_NULL_HANDLE || accelerationStructureAddress == 0)
	{
		return;
	}

	DescriptorEntry entry{};
	entry.Kind = EntryKind::PartitionedAccelerationStructure;
	entry.PartitionedAccelerationStructureAddress = accelerationStructureAddress;
	WriteEntries(descriptorSet, binding, std::span<const DescriptorEntry>(&entry, 1));
}

VkDescriptorType VulkanDescriptorAllocator::ToDescriptorType(EntryKind kind) noexcept
{
	switch (kind)
	{
		case EntryKind::SampledImage:
			return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case EntryKind::StorageImage:
			return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		case EntryKind::UniformBuffer:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case EntryKind::StorageBuffer:
			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		case EntryKind::Sampler:
			return VK_DESCRIPTOR_TYPE_SAMPLER;
		case EntryKind::AccelerationStructure:
			return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		case EntryKind::PartitionedAccelerationStructure:
			return VK_DESCRIPTOR_TYPE_PARTITIONED_ACCELERATION_STRUCTURE_NV;
		case EntryKind::Empty:
		default:
			return VK_DESCRIPTOR_TYPE_MAX_ENUM;
	}
}

VulkanDescriptorAllocator::EntryKind VulkanDescriptorAllocator::ToImageEntryKind(ERhiResourceViewKind viewKind) noexcept
{
	switch (viewKind)
	{
		case ERhiResourceViewKind::TextureUnorderedAccess:
			return EntryKind::StorageImage;
		case ERhiResourceViewKind::TextureShaderResource:
		default:
			return EntryKind::SampledImage;
	}
}

VulkanDescriptorAllocator::EntryKind VulkanDescriptorAllocator::ToBufferEntryKind(ERhiResourceViewKind viewKind) noexcept
{
	switch (viewKind)
	{
		case ERhiResourceViewKind::BufferUnorderedAccess:
			return EntryKind::StorageBuffer;
		case ERhiResourceViewKind::BufferShaderResource:
		default:
			return EntryKind::StorageBuffer;
	}
}

VkImageLayout VulkanDescriptorAllocator::ToImageLayout(EntryKind kind) noexcept
{
	return kind == EntryKind::StorageImage ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

VulkanDescriptorAllocator::DescriptorTableRecord* VulkanDescriptorAllocator::FindTableRecord(RhiDescriptorTableHandle tableHandle) noexcept
{
	std::uint32_t tableIndex = 0;
	std::uint16_t generation = 0;
	if (!tableHandle.Decode(tableIndex, generation) || tableIndex >= m_tables.size())
	{
		return nullptr;
	}

	DescriptorTableRecord& record = m_tables[tableIndex];
	return record.Allocated && record.Generation == generation ? &record : nullptr;
}

const VulkanDescriptorAllocator::DescriptorTableRecord* VulkanDescriptorAllocator::FindTableRecord(
    RhiDescriptorTableHandle tableHandle) const noexcept
{
	std::uint32_t tableIndex = 0;
	std::uint16_t generation = 0;
	if (!tableHandle.Decode(tableIndex, generation) || tableIndex >= m_tables.size())
	{
		return nullptr;
	}

	const DescriptorTableRecord& record = m_tables[tableIndex];
	return record.Allocated && record.Generation == generation ? &record : nullptr;
}

std::vector<VulkanDescriptorAllocator::DescriptorEntry>&
VulkanDescriptorAllocator::EditTableEntries(DescriptorTableRecord& record)
{
	if (record.Entries == nullptr)
	{
		record.Entries =
		    std::make_shared<std::vector<DescriptorEntry>>();
	}
	else if (record.Entries.use_count() != 1)
	{
		record.Entries =
		    std::make_shared<std::vector<DescriptorEntry>>(
		        *record.Entries);
	}

	return *record.Entries;
}

std::vector<VulkanDescriptorAllocator::DescriptorEntry>&
VulkanDescriptorAllocator::EditRegisteredDescriptors()
{
	if (m_registeredDescriptors == nullptr)
	{
		m_registeredDescriptors =
		    std::make_shared<std::vector<DescriptorEntry>>();
	}
	else if (m_registeredDescriptors.use_count() != 1)
	{
		m_registeredDescriptors =
		    std::make_shared<std::vector<DescriptorEntry>>(
		        *m_registeredDescriptors);
	}

	return *m_registeredDescriptors;
}

void VulkanDescriptorAllocator::RecycleTableRecord(std::uint32_t tableIndex) noexcept
{
	if (tableIndex >= m_tables.size())
	{
		return;
	}

	DescriptorTableRecord& record = m_tables[tableIndex];
	if (record.Allocated || record.Generation == RhiDescriptorTableHandle::MaximumGeneration)
	{
		return;
	}

	++record.Generation;
	m_freeTableIndices.push_back(tableIndex);
}

VulkanDescriptorAllocator::DescriptorEntry* VulkanDescriptorAllocator::FindRegisteredEntry(RhiGpuDescriptorHandle handle) noexcept
{
	if (m_registeredDescriptors == nullptr)
	{
		return nullptr;
	}

	std::uint32_t index = 0;
	if (!VulkanDescriptorHandles::DecodeGpuDescriptorHandle(handle, index) ||
	    index >= m_registeredDescriptors->size())
	{
		return nullptr;
	}
	return &(*m_registeredDescriptors)[index];
}

const VulkanDescriptorAllocator::DescriptorEntry* VulkanDescriptorAllocator::FindRegisteredEntry(
    RhiGpuDescriptorHandle handle) const noexcept
{
	if (m_registeredDescriptors == nullptr)
	{
		return nullptr;
	}

	std::uint32_t index = 0;
	if (!VulkanDescriptorHandles::DecodeGpuDescriptorHandle(handle, index) ||
	    index >= m_registeredDescriptors->size())
	{
		return nullptr;
	}
	return &(*m_registeredDescriptors)[index];
}

std::shared_ptr<const VulkanDescriptorAllocator::RecordingReadView>
VulkanDescriptorAllocator::GetRecordingReadView() const noexcept
{
	return m_recordingReadView.load(std::memory_order_acquire);
}

void VulkanDescriptorAllocator::CreateFallbackBuffer() noexcept
{
	if (m_fallbackBuffer != VK_NULL_HANDLE)
	{
		return;
	}

	VkBufferCreateInfo bufferInfo{
	    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .size = 256,
	    .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	    .queueFamilyIndexCount = 0,
	    .pQueueFamilyIndices = nullptr};
	m_rhi.ConfigureResourceQueueSharing(bufferInfo);
	if (vkCreateBuffer(m_rhi.GetDevice(), &bufferInfo, nullptr, &m_fallbackBuffer) != VK_SUCCESS || m_fallbackBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	VkMemoryRequirements memoryRequirements{};
	vkGetBufferMemoryRequirements(m_rhi.GetDevice(), m_fallbackBuffer, &memoryRequirements);
	VkPhysicalDeviceMemoryProperties memoryProperties{};
	vkGetPhysicalDeviceMemoryProperties(m_rhi.GetPhysicalDevice(), &memoryProperties);
	std::uint32_t memoryTypeIndex = UINT32_MAX;
	for (std::uint32_t index = 0; index < memoryProperties.memoryTypeCount; ++index)
	{
		const bool typeAvailable = (memoryRequirements.memoryTypeBits & (1u << index)) != 0;
		const bool flagsMatch = (memoryProperties.memoryTypes[index].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;
		if (typeAvailable && flagsMatch)
		{
			memoryTypeIndex = index;
			break;
		}
	}
	if (memoryTypeIndex == UINT32_MAX)
	{
		vkDestroyBuffer(m_rhi.GetDevice(), m_fallbackBuffer, nullptr);
		m_fallbackBuffer = VK_NULL_HANDLE;
		return;
	}

	const VkMemoryAllocateInfo allocateInfo{
	    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
	    .pNext = nullptr,
	    .allocationSize = memoryRequirements.size,
	    .memoryTypeIndex = memoryTypeIndex};
	if (vkAllocateMemory(m_rhi.GetDevice(), &allocateInfo, nullptr, &m_fallbackBufferMemory) != VK_SUCCESS ||
	    vkBindBufferMemory(m_rhi.GetDevice(), m_fallbackBuffer, m_fallbackBufferMemory, 0) != VK_SUCCESS)
	{
		if (m_fallbackBufferMemory != VK_NULL_HANDLE)
		{
			vkFreeMemory(m_rhi.GetDevice(), m_fallbackBufferMemory, nullptr);
			m_fallbackBufferMemory = VK_NULL_HANDLE;
		}
		vkDestroyBuffer(m_rhi.GetDevice(), m_fallbackBuffer, nullptr);
		m_fallbackBuffer = VK_NULL_HANDLE;
		return;
	}
}

void VulkanDescriptorAllocator::WriteFallbackDescriptors(
    VkDescriptorSet descriptorSet,
    const CompiledBinding* bindings,
    std::size_t bindingCount,
    std::uint32_t setIndex) noexcept
{
	if (descriptorSet == VK_NULL_HANDLE || bindings == nullptr)
	{
		return;
	}

	const VkBuffer fallbackBuffer = m_fallbackBuffer;
	if (fallbackBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	for (std::size_t index = 0; index < bindingCount; ++index)
	{
		const CompiledBinding& binding = bindings[index];
		if (binding.BindingPoint.Set != setIndex || binding.Type == CompiledBindingType::PushConstants ||
		    binding.Type == CompiledBindingType::SamplerTable)
		{
			continue;
		}

		if (binding.SemanticKind == ShaderParameterSemanticKind::ReadBuffer || binding.SemanticKind == ShaderParameterSemanticKind::RWBuffer)
		{
			DescriptorEntry entry{};
			entry.Kind = EntryKind::StorageBuffer;
			entry.Buffer = VkDescriptorBufferInfo{.buffer = fallbackBuffer, .offset = 0, .range = VK_WHOLE_SIZE};
			WriteEntries(descriptorSet, binding, std::span<const DescriptorEntry>(&entry, 1));
		}
		else if (binding.SemanticKind == ShaderParameterSemanticKind::UniformData)
		{
			DescriptorEntry entry{};
			entry.Kind = EntryKind::UniformBuffer;
			entry.Buffer = VkDescriptorBufferInfo{.buffer = fallbackBuffer, .offset = 0, .range = 256};
			WriteEntries(descriptorSet, binding, std::span<const DescriptorEntry>(&entry, 1));
		}
	}
}

bool VulkanDescriptorAllocator::EntryKindMatchesBinding(
    const CompiledBinding& binding,
    EntryKind entryKind) noexcept
{
	switch (binding.SemanticKind)
	{
		case ShaderParameterSemanticKind::ReadTexture:
			return entryKind == EntryKind::SampledImage;
		case ShaderParameterSemanticKind::ReadBuffer:
		case ShaderParameterSemanticKind::RWBuffer:
			return entryKind == EntryKind::StorageBuffer;
		case ShaderParameterSemanticKind::RWTexture:
			return entryKind == EntryKind::StorageImage;
		case ShaderParameterSemanticKind::UniformData:
			return entryKind == EntryKind::UniformBuffer;
		case ShaderParameterSemanticKind::SamplerSet:
			return entryKind == EntryKind::Sampler;
		case ShaderParameterSemanticKind::AccelerationStructure:
			return entryKind == EntryKind::AccelerationStructure ||
			       entryKind == EntryKind::PartitionedAccelerationStructure;
		default:
			return false;
	}
}

bool VulkanDescriptorAllocator::BuildWriteChunk(
    std::span<const DescriptorEntry> entries,
    EntryKind entryKind,
    DescriptorWriteChunk& outChunk) noexcept
{
	if (entries.empty() || entries.size() > DescriptorWriteChunkCapacity)
	{
		return false;
	}

	outChunk.Count = static_cast<std::uint32_t>(entries.size());
	for (std::size_t index = 0; index < entries.size(); ++index)
	{
		const DescriptorEntry& entry = entries[index];
		if (entry.Kind != entryKind)
		{
			return false;
		}

		switch (entryKind)
		{
			case EntryKind::SampledImage:
			case EntryKind::StorageImage:
			case EntryKind::Sampler:
				outChunk.ImageInfos[index] = entry.Image;
				break;
			case EntryKind::AccelerationStructure:
				outChunk.AccelerationStructures[index] = entry.AccelerationStructure;
				break;
			case EntryKind::PartitionedAccelerationStructure:
				outChunk.PartitionedAccelerationStructureAddresses[index] =
				    entry.PartitionedAccelerationStructureAddress;
				break;
			case EntryKind::UniformBuffer:
			case EntryKind::StorageBuffer:
				outChunk.BufferInfos[index] = entry.Buffer;
				break;
			case EntryKind::Empty:
			default:
				return false;
		}
	}

	return true;
}

void VulkanDescriptorAllocator::CommitWriteChunk(
    VkDescriptorSet descriptorSet,
    const CompiledBinding& binding,
    EntryKind entryKind,
    VkDescriptorType descriptorType,
    std::uint32_t firstDescriptor,
    const DescriptorWriteChunk& chunk) noexcept
{
	const VkWriteDescriptorSetAccelerationStructureKHR accelerationStructureWrite{
	    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
	    .pNext = nullptr,
	    .accelerationStructureCount = chunk.Count,
	    .pAccelerationStructures = chunk.AccelerationStructures.data()};
	const VkWriteDescriptorSetPartitionedAccelerationStructureNV partitionedAccelerationStructureWrite{
	    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_PARTITIONED_ACCELERATION_STRUCTURE_NV,
	    .pNext = nullptr,
	    .accelerationStructureCount = chunk.Count,
	    .pAccelerationStructures = chunk.PartitionedAccelerationStructureAddresses.data()};

	const bool writesImages =
	    entryKind == EntryKind::SampledImage ||
	    entryKind == EntryKind::StorageImage ||
	    entryKind == EntryKind::Sampler;
	const bool writesBuffers =
	    entryKind == EntryKind::UniformBuffer ||
	    entryKind == EntryKind::StorageBuffer;

	const VkWriteDescriptorSet write{
	    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	    .pNext = entryKind == EntryKind::AccelerationStructure
	                 ? static_cast<const void*>(&accelerationStructureWrite)
	                 : (entryKind == EntryKind::PartitionedAccelerationStructure
	                        ? static_cast<const void*>(&partitionedAccelerationStructureWrite)
	                        : nullptr),
	    .dstSet = descriptorSet,
	    .dstBinding = binding.BindingPoint.Binding,
	    .dstArrayElement = firstDescriptor,
	    .descriptorCount = chunk.Count,
	    .descriptorType = descriptorType,
	    .pImageInfo = writesImages ? chunk.ImageInfos.data() : nullptr,
	    .pBufferInfo = writesBuffers ? chunk.BufferInfos.data() : nullptr,
	    .pTexelBufferView = nullptr};

	vkUpdateDescriptorSets(m_rhi.GetDevice(), 1, &write, 0, nullptr);
}

void VulkanDescriptorAllocator::WriteEntries(
    VkDescriptorSet descriptorSet,
    const CompiledBinding& binding,
    std::span<const DescriptorEntry> entries) noexcept
{
	if (descriptorSet == VK_NULL_HANDLE || entries.empty())
	{
		return;
	}

	const EntryKind entryKind = entries.front().Kind;
	const VkDescriptorType descriptorType = ToDescriptorType(entryKind);
	if (descriptorType == VK_DESCRIPTOR_TYPE_MAX_ENUM)
	{
		return;
	}

	if (!EntryKindMatchesBinding(binding, entryKind))
	{
		return;
	}

	std::size_t firstDescriptor = 0;
	while (firstDescriptor < entries.size())
	{
		const std::size_t descriptorCount =
		    std::min(
		        DescriptorWriteChunkCapacity,
		        entries.size() - firstDescriptor);
		const std::span<const DescriptorEntry> entryChunk =
		    entries.subspan(firstDescriptor, descriptorCount);

		DescriptorWriteChunk chunk{};
		if (!BuildWriteChunk(entryChunk, entryKind, chunk))
		{
			return;
		}

		CommitWriteChunk(
		    descriptorSet,
		    binding,
		    entryKind,
		    descriptorType,
		    static_cast<std::uint32_t>(firstDescriptor),
		    chunk);
		firstDescriptor += chunk.Count;
	}
}
