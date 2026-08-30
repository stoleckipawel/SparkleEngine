#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Descriptors/VulkanDescriptorAllocator.h"

#include "Pipeline/RhiPipelineDesc.h"
#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Descriptors/VulkanDescriptorHandles.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Core/Public/Diagnostics/Verify.h"

#include <algorithm>
#include <array>
#include <format>

static const auto g_vulkanDescriptorAllocatorLogger = Logging::GetOrCreateLogger("RHI.Vulkan.DescriptorAllocator");

VulkanDescriptorAllocator::VulkanDescriptorAllocator(VulkanRhi& rhi) noexcept :
    m_rhi(rhi),
    m_registeredDescriptors(std::make_shared<std::vector<DescriptorEntry>>())
{
	PublishRecordingReadView();
}

RhiDescriptorAllocation VulkanDescriptorAllocator::AllocateDescriptor(ERhiDescriptorAllocatorType descriptorType)
{
	const RhiDescriptorTableHandle table = AllocateDescriptorTable(descriptorType, 1);
	return RhiDescriptorAllocation{.CpuHandle = GetDescriptorTableCpuHandle(table), .GpuHandle = {}};
}

void VulkanDescriptorAllocator::BeginFrame(std::uint32_t frameIndex) noexcept
{
	if (frameIndex >= m_retiredTableIndices.size())
	{
		return;
	}

	{
		std::scoped_lock lock(m_registryMutex);
		for (const std::uint32_t tableIndex : m_retiredTableIndices[frameIndex])
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
			    DescriptorTableReadRecord{.Entries = table.Entries, .Allocated = table.Allocated, .Generation = table.Generation});
		}
		readView->RegisteredDescriptors = m_registeredDescriptors;
	}

	m_recordingReadView.store(std::shared_ptr<const RecordingReadView>(std::move(readView)), std::memory_order_release);
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
	record.Entries = std::make_shared<std::vector<DescriptorEntry>>(descriptorCount, DescriptorEntry{});
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
	if (record == nullptr || record->Entries == nullptr || descriptorIndex >= record->Entries->size())
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
	return VulkanDescriptorHandles::MakeGpuDescriptorHandle(static_cast<std::uint32_t>(descriptors.size() - 1));
}

RhiGpuDescriptorHandle VulkanDescriptorAllocator::RegisterBufferDescriptor(
    ERhiResourceViewKind viewKind,
    VkBuffer buffer,
    std::uint64_t offsetInBytes,
    std::uint64_t sizeInBytes)
{
	if (buffer == VK_NULL_HANDLE || sizeInBytes == 0)
	{
		return {};
	}

	DescriptorEntry entry{};
	entry.Kind = ToBufferEntryKind(viewKind);
	entry.Buffer = VkDescriptorBufferInfo{.buffer = buffer, .offset = offsetInBytes, .range = sizeInBytes};

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
	return VulkanDescriptorHandles::MakeGpuDescriptorHandle(static_cast<std::uint32_t>(descriptors.size() - 1));
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

void VulkanDescriptorAllocator::WriteSamplerDescriptor(RhiCpuDescriptorHandle destination, VkSampler sampler) noexcept
{
	RhiDescriptorTableHandle tableHandle{};
	std::uint32_t descriptorIndex = 0;
	if (!VulkanDescriptorHandles::DecodeCpuDescriptorHandle(destination, tableHandle, descriptorIndex) || sampler == VK_NULL_HANDLE)
	{
		Diagnostics::Fatal(
		    g_vulkanDescriptorAllocatorLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan sampler write requires a valid destination descriptor and sampler.");
	}

	std::scoped_lock lock(m_registryMutex);
	DescriptorTableRecord* const record = FindTableRecord(tableHandle);
	if (record == nullptr || record->Entries == nullptr || descriptorIndex >= record->Entries->size())
	{
		Diagnostics::Fatal(
		    g_vulkanDescriptorAllocatorLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan sampler write references an unavailable descriptor table entry.");
	}
	std::vector<DescriptorEntry>& entries = EditTableEntries(*record);
	DescriptorEntry& entry = entries[descriptorIndex];
	entry.Kind = EntryKind::Sampler;
	entry.Image = VkDescriptorImageInfo{.sampler = sampler, .imageView = VK_NULL_HANDLE, .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED};
}

bool VulkanDescriptorAllocator::WriteRegisteredDescriptor(RhiCpuDescriptorHandle destination, RhiGpuDescriptorHandle source) noexcept
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
	if (table == nullptr || table->Entries == nullptr || sourceEntry == nullptr || sourceEntry->Kind == EntryKind::Empty
	    || descriptorIndex >= table->Entries->size())
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
		Diagnostics::Fatal(
		    g_vulkanDescriptorAllocatorLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan descriptor-table binding requires a descriptor set and table binding.");
	}

	const std::shared_ptr<const RecordingReadView> readView = GetRecordingReadView();
	std::uint32_t tableIndex = 0;
	std::uint16_t generation = 0;
	if (readView == nullptr || !tableBinding.Table.Decode(tableIndex, generation) || tableIndex >= readView->Tables.size())
	{
		Diagnostics::Fatal(
		    g_vulkanDescriptorAllocatorLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan descriptor-table binding cannot resolve its recording snapshot.");
	}

	const DescriptorTableReadRecord& table = readView->Tables[tableIndex];
	if (!table.Allocated || table.Generation != generation || table.Entries == nullptr
	    || tableBinding.DescriptorIndex >= table.Entries->size())
	{
		Diagnostics::Fatal(
		    g_vulkanDescriptorAllocatorLogger,
		    __FILE__,
		    __LINE__,
		    std::format(
		        "Vulkan descriptor-table binding '{}' references stale or unavailable table {} generation {} "
		        "(snapshot allocated={}, generation={}, descriptor count={}, first descriptor={}).",
		        binding.Name != nullptr ? binding.Name : "<unnamed>",
		        tableIndex,
		        generation,
		        table.Allocated,
		        table.Generation,
		        table.Entries != nullptr ? table.Entries->size() : 0u,
		        tableBinding.DescriptorIndex));
	}

	const std::size_t availableDescriptorCount = table.Entries->size() - tableBinding.DescriptorIndex;
	if (binding.DescriptorCount == 0 || availableDescriptorCount == 0
	    || (!binding.Bindless.BindlessEligible && availableDescriptorCount < binding.DescriptorCount))
	{
		Diagnostics::Fatal(
		    g_vulkanDescriptorAllocatorLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan descriptor table does not contain the binding's complete descriptor range.");
	}
	const std::size_t descriptorCount = std::min<std::size_t>(availableDescriptorCount, binding.DescriptorCount);
	WriteEntries(
	    descriptorSet,
	    binding,
	    std::span<const DescriptorEntry>(table.Entries->data() + tableBinding.DescriptorIndex, descriptorCount));
}

void VulkanDescriptorAllocator::WriteDescriptorHandle(
    VkDescriptorSet descriptorSet,
    const CompiledBinding& binding,
    RhiGpuDescriptorHandle handle) noexcept
{
	if (descriptorSet == VK_NULL_HANDLE || !handle)
	{
		Diagnostics::Fatal(
		    g_vulkanDescriptorAllocatorLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan descriptor-handle binding requires a descriptor set and registered handle.");
	}

	const std::shared_ptr<const RecordingReadView> readView = GetRecordingReadView();
	std::uint32_t descriptorIndex = 0;
	if (readView == nullptr || readView->RegisteredDescriptors == nullptr
	    || !VulkanDescriptorHandles::DecodeGpuDescriptorHandle(handle, descriptorIndex)
	    || descriptorIndex >= readView->RegisteredDescriptors->size())
	{
		Diagnostics::Fatal(
		    g_vulkanDescriptorAllocatorLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan descriptor-handle binding cannot resolve its recording snapshot.");
	}

	const DescriptorEntry& entry = (*readView->RegisteredDescriptors)[descriptorIndex];
	if (entry.Kind == EntryKind::Empty)
	{
		Diagnostics::Fatal(g_vulkanDescriptorAllocatorLogger, __FILE__, __LINE__, "Vulkan descriptor handle references a released entry.");
	}

	WriteEntries(descriptorSet, binding, std::span<const DescriptorEntry>(&entry, 1));
}

void VulkanDescriptorAllocator::WriteBufferDescriptor(
    VkDescriptorSet descriptorSet,
    const CompiledBinding& binding,
    VkBuffer buffer,
    VkDeviceSize offset,
    VkDeviceSize range) noexcept
{
	if (descriptorSet == VK_NULL_HANDLE || buffer == VK_NULL_HANDLE || range == 0)
	{
		Diagnostics::Fatal(
		    g_vulkanDescriptorAllocatorLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan buffer binding requires a descriptor set, resolved buffer, and non-zero range.");
	}

	DescriptorEntry entry{};
	entry.Kind = binding.Type == CompiledBindingType::ConstantBuffer ? EntryKind::UniformBuffer : EntryKind::StorageBuffer;
	entry.Buffer = VkDescriptorBufferInfo{.buffer = buffer, .offset = offset, .range = range};
	WriteEntries(descriptorSet, binding, std::span<const DescriptorEntry>(&entry, 1));
}

void VulkanDescriptorAllocator::WriteAccelerationStructureDescriptor(
    VkDescriptorSet descriptorSet,
    const CompiledBinding& binding,
    VkAccelerationStructureKHR accelerationStructure) noexcept
{
	if (descriptorSet == VK_NULL_HANDLE || accelerationStructure == VK_NULL_HANDLE)
	{
		Diagnostics::Fatal(
		    g_vulkanDescriptorAllocatorLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan acceleration-structure binding requires a descriptor set and resolved acceleration structure.");
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
		Diagnostics::Fatal(
		    g_vulkanDescriptorAllocatorLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan partitioned acceleration-structure binding requires a descriptor set and GPU address.");
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
			return EntryKind::SampledImage;
		default:
			Diagnostics::Fatal(g_vulkanDescriptorAllocatorLogger, __FILE__, __LINE__, "Unsupported Vulkan image descriptor kind.");
	}
}

VulkanDescriptorAllocator::EntryKind VulkanDescriptorAllocator::ToBufferEntryKind(ERhiResourceViewKind viewKind) noexcept
{
	switch (viewKind)
	{
		case ERhiResourceViewKind::BufferUnorderedAccess:
			return EntryKind::StorageBuffer;
		case ERhiResourceViewKind::BufferShaderResource:
			return EntryKind::StorageBuffer;
		default:
			Diagnostics::Fatal(g_vulkanDescriptorAllocatorLogger, __FILE__, __LINE__, "Unsupported Vulkan buffer descriptor kind.");
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

std::vector<VulkanDescriptorAllocator::DescriptorEntry>& VulkanDescriptorAllocator::EditTableEntries(DescriptorTableRecord& record)
{
	if (record.Entries == nullptr)
	{
		record.Entries = std::make_shared<std::vector<DescriptorEntry>>();
	}
	else if (record.Entries.use_count() != 1)
	{
		record.Entries = std::make_shared<std::vector<DescriptorEntry>>(*record.Entries);
	}

	return *record.Entries;
}

std::vector<VulkanDescriptorAllocator::DescriptorEntry>& VulkanDescriptorAllocator::EditRegisteredDescriptors()
{
	if (m_registeredDescriptors == nullptr)
	{
		m_registeredDescriptors = std::make_shared<std::vector<DescriptorEntry>>();
	}
	else if (m_registeredDescriptors.use_count() != 1)
	{
		m_registeredDescriptors = std::make_shared<std::vector<DescriptorEntry>>(*m_registeredDescriptors);
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
	if (!VulkanDescriptorHandles::DecodeGpuDescriptorHandle(handle, index) || index >= m_registeredDescriptors->size())
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
	if (!VulkanDescriptorHandles::DecodeGpuDescriptorHandle(handle, index) || index >= m_registeredDescriptors->size())
	{
		return nullptr;
	}
	return &(*m_registeredDescriptors)[index];
}

std::shared_ptr<const VulkanDescriptorAllocator::RecordingReadView> VulkanDescriptorAllocator::GetRecordingReadView() const noexcept
{
	return m_recordingReadView.load(std::memory_order_acquire);
}

bool VulkanDescriptorAllocator::EntryKindMatchesBinding(const CompiledBinding& binding, EntryKind entryKind) noexcept
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
			return entryKind == EntryKind::AccelerationStructure || entryKind == EntryKind::PartitionedAccelerationStructure;
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
				outChunk.PartitionedAccelerationStructureAddresses[index] = entry.PartitionedAccelerationStructureAddress;
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
	    entryKind == EntryKind::SampledImage || entryKind == EntryKind::StorageImage || entryKind == EntryKind::Sampler;
	const bool writesBuffers = entryKind == EntryKind::UniformBuffer || entryKind == EntryKind::StorageBuffer;

	const VkWriteDescriptorSet write{
	    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	    .pNext = entryKind == EntryKind::AccelerationStructure
	        ? static_cast<const void*>(&accelerationStructureWrite)
	        : (entryKind == EntryKind::PartitionedAccelerationStructure ? static_cast<const void*>(&partitionedAccelerationStructureWrite)
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
		Diagnostics::Fatal(
		    g_vulkanDescriptorAllocatorLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan descriptor write requires a descriptor set and at least one descriptor.");
	}
	if (entries.size() > binding.DescriptorCount || (!binding.Bindless.BindlessEligible && entries.size() != binding.DescriptorCount))
	{
		Diagnostics::Fatal(
		    g_vulkanDescriptorAllocatorLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan descriptor write cardinality does not match the compiled shader binding.");
	}

	const EntryKind entryKind = entries.front().Kind;
	const VkDescriptorType descriptorType = ToDescriptorType(entryKind);
	if (descriptorType == VK_DESCRIPTOR_TYPE_MAX_ENUM)
	{
		Diagnostics::Fatal(g_vulkanDescriptorAllocatorLogger, __FILE__, __LINE__, "Vulkan descriptor write contains an empty entry.");
	}

	if (!EntryKindMatchesBinding(binding, entryKind))
	{
		Diagnostics::Fatal(
		    g_vulkanDescriptorAllocatorLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan descriptor entry kind does not match the compiled shader binding.");
	}

	std::size_t firstDescriptor = 0;
	while (firstDescriptor < entries.size())
	{
		const std::size_t descriptorCount = std::min(DescriptorWriteChunkCapacity, entries.size() - firstDescriptor);
		const std::span<const DescriptorEntry> entryChunk = entries.subspan(firstDescriptor, descriptorCount);

		DescriptorWriteChunk chunk{};
		if (!BuildWriteChunk(entryChunk, entryKind, chunk))
		{
			Diagnostics::Fatal(
			    g_vulkanDescriptorAllocatorLogger,
			    __FILE__,
			    __LINE__,
			    "Vulkan descriptor array contains inconsistent or unsupported entries.");
		}

		CommitWriteChunk(descriptorSet, binding, entryKind, descriptorType, static_cast<std::uint32_t>(firstDescriptor), chunk);
		firstDescriptor += chunk.Count;
	}
}
