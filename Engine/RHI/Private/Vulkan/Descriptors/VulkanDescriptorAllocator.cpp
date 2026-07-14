#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Descriptors/VulkanDescriptorAllocator.h"

#include "Pipeline/RhiPipelineStateDesc.h"
#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Descriptors/VulkanDescriptorHandles.h"
#include "Vulkan/Device/VulkanRhi.h"

#include <algorithm>
#include <array>

static const auto g_vulkanDescriptorAllocatorLogger = Logging::GetOrCreateLogger("RHI.Vulkan.Descriptors");

namespace VulkanDescriptorAllocatorDiagnostics
{
	const char* ToString(ShaderParameterSemanticKind semanticKind) noexcept
	{
		switch (semanticKind)
		{
			case ShaderParameterSemanticKind::ReadTexture:
				return "ReadTexture";
			case ShaderParameterSemanticKind::ReadBuffer:
				return "ReadBuffer";
			case ShaderParameterSemanticKind::RWTexture:
				return "RWTexture";
			case ShaderParameterSemanticKind::RWBuffer:
				return "RWBuffer";
			case ShaderParameterSemanticKind::UniformData:
				return "UniformData";
			case ShaderParameterSemanticKind::SamplerSet:
				return "SamplerSet";
			case ShaderParameterSemanticKind::AccelerationStructure:
				return "AccelerationStructure";
			case ShaderParameterSemanticKind::RenderTarget:
				return "RenderTarget";
			case ShaderParameterSemanticKind::DepthTarget:
				return "DepthTarget";
			default:
				return "Unknown";
		}
	}

}  // namespace VulkanDescriptorAllocatorDiagnostics

VulkanDescriptorAllocator::VulkanDescriptorAllocator(VulkanRhi& rhi) noexcept : m_rhi(rhi) {}

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
	for (std::vector<DescriptorPoolPage>& poolPages : m_framePoolPages)
	{
		for (DescriptorPoolPage& page : poolPages)
		{
			if (page.Pool != VK_NULL_HANDLE)
			{
				vkDestroyDescriptorPool(m_rhi.GetDevice(), page.Pool, nullptr);
			}
		}
	}
}

RhiDescriptorAllocation VulkanDescriptorAllocator::AllocateDescriptor(ERhiDescriptorAllocatorType descriptorType)
{
	const RhiDescriptorTableHandle table = AllocateDescriptorTable(descriptorType, 1);
	return RhiDescriptorAllocation{.CpuHandle = GetDescriptorTableCpuHandle(table), .GpuHandle = {}};
}

void VulkanDescriptorAllocator::BeginFrame(std::uint32_t frameIndex) noexcept
{
	std::scoped_lock lock(m_mutex);
	m_currentFrameIndex = frameIndex;
	if (m_framePoolPages.size() <= frameIndex)
	{
		m_framePoolPages.resize(static_cast<std::size_t>(frameIndex) + 1u);
	}

	for (DescriptorPoolPage& page : m_framePoolPages[frameIndex])
	{
		if (page.Pool == VK_NULL_HANDLE)
		{
			continue;
		}
		const VkResult result = vkResetDescriptorPool(m_rhi.GetDevice(), page.Pool, 0);
		if (VulkanResult::Succeeded(result))
		{
			page.AllocatedSets = 0;
			page.Remaining = page.Capacity;
		}
	}
}

void VulkanDescriptorAllocator::ReleaseDescriptor(ERhiDescriptorAllocatorType, const RhiDescriptorAllocation& allocation) noexcept
{
	std::uint32_t tableIndex = 0;
	std::uint32_t descriptorIndex = 0;
	if (!VulkanDescriptorHandles::DecodeCpuDescriptorHandle(allocation.CpuHandle, tableIndex, descriptorIndex) || descriptorIndex != 0)
	{
		return;
	}
	ReleaseDescriptorTable(VulkanDescriptorHandles::MakeTableHandle(tableIndex));
}

RhiDescriptorTableHandle VulkanDescriptorAllocator::AllocateDescriptorTable(
    ERhiDescriptorAllocatorType descriptorType,
    std::uint32_t descriptorCount)
{
	if (descriptorCount == 0)
	{
		return {};
	}

	std::scoped_lock lock(m_mutex);
	DescriptorTableRecord record{};
	record.Type = descriptorType;
	record.Entries.assign(descriptorCount, DescriptorEntry{});
	record.Allocated = true;

	if (!m_freeTableIndices.empty())
	{
		const std::uint32_t index = m_freeTableIndices.back();
		m_freeTableIndices.pop_back();
		m_tables[index] = std::move(record);
		return VulkanDescriptorHandles::MakeTableHandle(index);
	}

	m_tables.push_back(std::move(record));
	return VulkanDescriptorHandles::MakeTableHandle(static_cast<std::uint32_t>(m_tables.size() - 1));
}

RhiCpuDescriptorHandle VulkanDescriptorAllocator::GetDescriptorTableCpuHandle(
    RhiDescriptorTableHandle tableHandle,
    std::uint32_t descriptorIndex) const noexcept
{
	std::scoped_lock lock(m_mutex);
	const DescriptorTableRecord* const record = FindTableRecord(tableHandle);
	if (record == nullptr || descriptorIndex >= record->Entries.size())
	{
		return {};
	}
	return VulkanDescriptorHandles::MakeCpuDescriptorHandle(tableHandle.Value - 1u, descriptorIndex);
}

void VulkanDescriptorAllocator::ReleaseDescriptorTable(RhiDescriptorTableHandle tableHandle) noexcept
{
	std::scoped_lock lock(m_mutex);
	DescriptorTableRecord* const record = FindTableRecord(tableHandle);
	if (record == nullptr)
	{
		return;
	}
	*record = DescriptorTableRecord{};
	m_freeTableIndices.push_back(tableHandle.Value - 1u);
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

	std::scoped_lock lock(m_mutex);
	if (!m_freeRegisteredDescriptorIndices.empty())
	{
		const std::uint32_t index = m_freeRegisteredDescriptorIndices.back();
		m_freeRegisteredDescriptorIndices.pop_back();
		m_registeredDescriptors[index] = entry;
		return VulkanDescriptorHandles::MakeGpuDescriptorHandle(index);
	}

	m_registeredDescriptors.push_back(entry);
	return VulkanDescriptorHandles::MakeGpuDescriptorHandle(static_cast<std::uint32_t>(m_registeredDescriptors.size() - 1));
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

	std::scoped_lock lock(m_mutex);
	if (!m_freeRegisteredDescriptorIndices.empty())
	{
		const std::uint32_t index = m_freeRegisteredDescriptorIndices.back();
		m_freeRegisteredDescriptorIndices.pop_back();
		m_registeredDescriptors[index] = entry;
		return VulkanDescriptorHandles::MakeGpuDescriptorHandle(index);
	}

	m_registeredDescriptors.push_back(entry);
	return VulkanDescriptorHandles::MakeGpuDescriptorHandle(static_cast<std::uint32_t>(m_registeredDescriptors.size() - 1));
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

	std::scoped_lock lock(m_mutex);
	if (!m_freeRegisteredDescriptorIndices.empty())
	{
		const std::uint32_t index = m_freeRegisteredDescriptorIndices.back();
		m_freeRegisteredDescriptorIndices.pop_back();
		m_registeredDescriptors[index] = entry;
		return VulkanDescriptorHandles::MakeGpuDescriptorHandle(index);
	}

	m_registeredDescriptors.push_back(entry);
	return VulkanDescriptorHandles::MakeGpuDescriptorHandle(static_cast<std::uint32_t>(m_registeredDescriptors.size() - 1));
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

	std::scoped_lock lock(m_mutex);
	if (!m_freeRegisteredDescriptorIndices.empty())
	{
		const std::uint32_t index = m_freeRegisteredDescriptorIndices.back();
		m_freeRegisteredDescriptorIndices.pop_back();
		m_registeredDescriptors[index] = entry;
		return VulkanDescriptorHandles::MakeGpuDescriptorHandle(index);
	}

	m_registeredDescriptors.push_back(entry);
	return VulkanDescriptorHandles::MakeGpuDescriptorHandle(static_cast<std::uint32_t>(m_registeredDescriptors.size() - 1));
}

void VulkanDescriptorAllocator::ReleaseRegisteredDescriptor(RhiGpuDescriptorHandle handle) noexcept
{
	std::uint32_t index = 0;
	if (!VulkanDescriptorHandles::DecodeGpuDescriptorHandle(handle, index))
	{
		return;
	}

	std::scoped_lock lock(m_mutex);
	if (index >= m_registeredDescriptors.size())
	{
		return;
	}
	if (m_registeredDescriptors[index].Kind == EntryKind::Empty)
	{
		return;
	}
	m_registeredDescriptors[index] = DescriptorEntry{};
	m_freeRegisteredDescriptorIndices.push_back(index);
}

void VulkanDescriptorAllocator::WriteImageDescriptor(
    RhiCpuDescriptorHandle destination,
    ERhiResourceViewKind viewKind,
    VkImageView imageView) noexcept
{
	std::uint32_t tableIndex = 0;
	std::uint32_t descriptorIndex = 0;
	if (!VulkanDescriptorHandles::DecodeCpuDescriptorHandle(destination, tableIndex, descriptorIndex) || imageView == VK_NULL_HANDLE)
	{
		return;
	}

	std::scoped_lock lock(m_mutex);
	if (tableIndex >= m_tables.size() || descriptorIndex >= m_tables[tableIndex].Entries.size())
	{
		return;
	}
	DescriptorEntry& entry = m_tables[tableIndex].Entries[descriptorIndex];
	entry.Kind = ToImageEntryKind(viewKind);
	entry.Image = VkDescriptorImageInfo{.sampler = VK_NULL_HANDLE, .imageView = imageView, .imageLayout = ToImageLayout(entry.Kind)};
}

void VulkanDescriptorAllocator::WriteSamplerDescriptor(RhiCpuDescriptorHandle destination, VkSampler sampler) noexcept
{
	std::uint32_t tableIndex = 0;
	std::uint32_t descriptorIndex = 0;
	if (!VulkanDescriptorHandles::DecodeCpuDescriptorHandle(destination, tableIndex, descriptorIndex) || sampler == VK_NULL_HANDLE)
	{
		return;
	}

	std::scoped_lock lock(m_mutex);
	if (tableIndex >= m_tables.size() || descriptorIndex >= m_tables[tableIndex].Entries.size())
	{
		return;
	}
	DescriptorEntry& entry = m_tables[tableIndex].Entries[descriptorIndex];
	entry.Kind = EntryKind::Sampler;
	entry.Image = VkDescriptorImageInfo{.sampler = sampler, .imageView = VK_NULL_HANDLE, .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED};
}

VkDescriptorSet VulkanDescriptorAllocator::AllocateTransientSet(
    VkDescriptorSetLayout layout,
    const CompiledBinding* bindings,
    std::size_t bindingCount,
    std::uint32_t setIndex)
{
	if (layout == VK_NULL_HANDLE)
	{
		return VK_NULL_HANDLE;
	}

	std::scoped_lock lock(m_mutex);
	if (m_framePoolPages.size() <= m_currentFrameIndex)
	{
		m_framePoolPages.resize(static_cast<std::size_t>(m_currentFrameIndex) + 1u);
	}
	const DescriptorCounts requirements = GetPoolRequirements(bindings, bindingCount, setIndex);

	std::vector<DescriptorPoolPage>& poolPages = m_framePoolPages[m_currentFrameIndex];
	for (DescriptorPoolPage& page : poolPages)
	{
		if (page.AllocatedSets >= DescriptorSetsPerPage || !CanAllocateFromPage(page, requirements))
		{
			continue;
		}

		const VkDescriptorSetAllocateInfo allocateInfo{
		    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		    .pNext = nullptr,
		    .descriptorPool = page.Pool,
		    .descriptorSetCount = 1,
		    .pSetLayouts = &layout};
		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		const VkResult result = vkAllocateDescriptorSets(m_rhi.GetDevice(), &allocateInfo, &descriptorSet);
		if (VulkanResult::Succeeded(result))
		{
			++page.AllocatedSets;
			ConsumePoolCapacity(page, requirements);
			return descriptorSet;
		}
	}

	DescriptorPoolPage page{};
	constexpr DescriptorCounts defaultCapacity = {1024, 1024, 1024, 512, 256, 128};
	for (std::size_t index = 0; index < page.Capacity.size(); ++index)
	{
		page.Capacity[index] = std::max(defaultCapacity[index], requirements[index]);
	}
	page.Remaining = page.Capacity;
	page.Pool = CreatePoolPage(page.Capacity);
	if (page.Pool == VK_NULL_HANDLE)
	{
		return VK_NULL_HANDLE;
	}

	const VkDescriptorSetAllocateInfo allocateInfo{
	    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
	    .pNext = nullptr,
	    .descriptorPool = page.Pool,
	    .descriptorSetCount = 1,
	    .pSetLayouts = &layout};
	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
	const VkResult result = vkAllocateDescriptorSets(m_rhi.GetDevice(), &allocateInfo, &descriptorSet);
	if (!VulkanResult::Succeeded(result))
	{
		vkDestroyDescriptorPool(m_rhi.GetDevice(), page.Pool, nullptr);
		return VK_NULL_HANDLE;
	}
	++page.AllocatedSets;
	ConsumePoolCapacity(page, requirements);
	poolPages.push_back(page);
	return descriptorSet;
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

	std::vector<DescriptorEntry> entries;
	{
		std::scoped_lock lock(m_mutex);
		const DescriptorTableRecord* const record = FindTableRecord(tableBinding.Table);
		if (record == nullptr || tableBinding.DescriptorIndex >= record->Entries.size())
		{
			return;
		}

		const std::uint32_t count =
		    std::min(binding.DescriptorCount, static_cast<std::uint32_t>(record->Entries.size() - tableBinding.DescriptorIndex));
		entries.assign(
		    record->Entries.begin() + tableBinding.DescriptorIndex,
		    record->Entries.begin() + tableBinding.DescriptorIndex + count);
	}
	WriteEntries(descriptorSet, binding, entries);
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

	DescriptorEntry entry{};
	{
		std::scoped_lock lock(m_mutex);
		const DescriptorEntry* const registeredEntry = FindRegisteredEntry(handle);
		if (registeredEntry == nullptr || registeredEntry->Kind == EntryKind::Empty)
		{
			return;
		}
		entry = *registeredEntry;
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
	if (!tableHandle || tableHandle.Value - 1u >= m_tables.size() || !m_tables[tableHandle.Value - 1u].Allocated)
	{
		return nullptr;
	}
	return &m_tables[tableHandle.Value - 1u];
}

const VulkanDescriptorAllocator::DescriptorTableRecord* VulkanDescriptorAllocator::FindTableRecord(
    RhiDescriptorTableHandle tableHandle) const noexcept
{
	if (!tableHandle || tableHandle.Value - 1u >= m_tables.size() || !m_tables[tableHandle.Value - 1u].Allocated)
	{
		return nullptr;
	}
	return &m_tables[tableHandle.Value - 1u];
}

VulkanDescriptorAllocator::DescriptorEntry* VulkanDescriptorAllocator::FindRegisteredEntry(RhiGpuDescriptorHandle handle) noexcept
{
	std::uint32_t index = 0;
	if (!VulkanDescriptorHandles::DecodeGpuDescriptorHandle(handle, index) || index >= m_registeredDescriptors.size())
	{
		return nullptr;
	}
	return &m_registeredDescriptors[index];
}

const VulkanDescriptorAllocator::DescriptorEntry* VulkanDescriptorAllocator::FindRegisteredEntry(
    RhiGpuDescriptorHandle handle) const noexcept
{
	std::uint32_t index = 0;
	if (!VulkanDescriptorHandles::DecodeGpuDescriptorHandle(handle, index) || index >= m_registeredDescriptors.size())
	{
		return nullptr;
	}
	return &m_registeredDescriptors[index];
}

VulkanDescriptorAllocator::DescriptorCounts VulkanDescriptorAllocator::GetPoolRequirements(
    const CompiledBinding* bindings,
    std::size_t bindingCount,
    std::uint32_t setIndex) noexcept
{
	DescriptorCounts requirements = {};
	if (bindings == nullptr)
	{
		return requirements;
	}
	for (std::size_t bindingIndex = 0; bindingIndex < bindingCount; ++bindingIndex)
	{
		const CompiledBinding& binding = bindings[bindingIndex];
		if (binding.BindingPoint.Set != setIndex || binding.Type == CompiledBindingType::PushConstants)
		{
			continue;
		}

		PoolClass poolClass = PoolClass::Count;
		switch (binding.SemanticKind)
		{
			case ShaderParameterSemanticKind::UniformData:
				poolClass = PoolClass::UniformBuffer;
				break;
			case ShaderParameterSemanticKind::ReadBuffer:
			case ShaderParameterSemanticKind::RWBuffer:
				poolClass = PoolClass::StorageBuffer;
				break;
			case ShaderParameterSemanticKind::ReadTexture:
				poolClass = PoolClass::SampledImage;
				break;
			case ShaderParameterSemanticKind::RWTexture:
				poolClass = PoolClass::StorageImage;
				break;
			case ShaderParameterSemanticKind::SamplerSet:
				poolClass = PoolClass::Sampler;
				break;
			case ShaderParameterSemanticKind::AccelerationStructure:
				poolClass = PoolClass::AccelerationStructure;
				break;
			default:
				break;
		}
		if (poolClass != PoolClass::Count)
		{
			const std::size_t poolIndex = static_cast<std::size_t>(poolClass);
			requirements[poolIndex] += binding.DescriptorCount;
		}
	}
	return requirements;
}

bool VulkanDescriptorAllocator::CanAllocateFromPage(
    const DescriptorPoolPage& page,
    const DescriptorCounts& requirements) noexcept
{
	for (std::size_t index = 0; index < requirements.size(); ++index)
	{
		if (requirements[index] > page.Remaining[index])
		{
			return false;
		}
	}
	return true;
}

void VulkanDescriptorAllocator::ConsumePoolCapacity(
    DescriptorPoolPage& page,
    const DescriptorCounts& requirements) noexcept
{
	for (std::size_t index = 0; index < requirements.size(); ++index)
	{
		page.Remaining[index] -= requirements[index];
	}
}

VkDescriptorPool VulkanDescriptorAllocator::CreatePoolPage(const DescriptorCounts& capacity)
{
	const std::array<VkDescriptorPoolSize, DescriptorPoolTypeCount> poolSizes{
	    VkDescriptorPoolSize{
	        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        .descriptorCount = capacity[static_cast<std::size_t>(PoolClass::UniformBuffer)]},
	    VkDescriptorPoolSize{
	        .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	        .descriptorCount = capacity[static_cast<std::size_t>(PoolClass::StorageBuffer)]},
	    VkDescriptorPoolSize{
	        .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
	        .descriptorCount = capacity[static_cast<std::size_t>(PoolClass::SampledImage)]},
	    VkDescriptorPoolSize{
	        .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
	        .descriptorCount = capacity[static_cast<std::size_t>(PoolClass::StorageImage)]},
	    VkDescriptorPoolSize{
	        .type = VK_DESCRIPTOR_TYPE_SAMPLER,
	        .descriptorCount = capacity[static_cast<std::size_t>(PoolClass::Sampler)]},
	    VkDescriptorPoolSize{
	        .type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
	        .descriptorCount = capacity[static_cast<std::size_t>(PoolClass::AccelerationStructure)]}};
	const VkDescriptorPoolCreateInfo createInfo{
	    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
	    .maxSets = DescriptorSetsPerPage,
	    .poolSizeCount = static_cast<std::uint32_t>(poolSizes.size()),
	    .pPoolSizes = poolSizes.data()};
	VkDescriptorPool pool = VK_NULL_HANDLE;
	const VkResult result = vkCreateDescriptorPool(m_rhi.GetDevice(), &createInfo, nullptr, &pool);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fail(
		    g_vulkanDescriptorAllocatorLogger,
		    __FILE__,
		    __LINE__,
		    VulkanResult::FormatFailure("vkCreateDescriptorPool", result));
	}
	return pool;
}

VkBuffer VulkanDescriptorAllocator::EnsureFallbackBuffer() noexcept
{
	if (m_fallbackBuffer != VK_NULL_HANDLE)
	{
		return m_fallbackBuffer;
	}

	const VkBufferCreateInfo bufferInfo{
	    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .size = 256,
	    .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	    .queueFamilyIndexCount = 0,
	    .pQueueFamilyIndices = nullptr};
	if (vkCreateBuffer(m_rhi.GetDevice(), &bufferInfo, nullptr, &m_fallbackBuffer) != VK_SUCCESS || m_fallbackBuffer == VK_NULL_HANDLE)
	{
		return VK_NULL_HANDLE;
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
		return VK_NULL_HANDLE;
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
		return VK_NULL_HANDLE;
	}

	return m_fallbackBuffer;
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

	const VkBuffer fallbackBuffer = EnsureFallbackBuffer();
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

	const bool entryMatchesBinding =
	    (binding.SemanticKind == ShaderParameterSemanticKind::ReadTexture && entryKind == EntryKind::SampledImage) ||
	    ((binding.SemanticKind == ShaderParameterSemanticKind::ReadBuffer || binding.SemanticKind == ShaderParameterSemanticKind::RWBuffer) &&
	     entryKind == EntryKind::StorageBuffer) ||
	    (binding.SemanticKind == ShaderParameterSemanticKind::RWTexture && entryKind == EntryKind::StorageImage) ||
	    (binding.SemanticKind == ShaderParameterSemanticKind::UniformData && entryKind == EntryKind::UniformBuffer) ||
	    (binding.SemanticKind == ShaderParameterSemanticKind::SamplerSet && entryKind == EntryKind::Sampler) ||
	    (binding.SemanticKind == ShaderParameterSemanticKind::AccelerationStructure &&
	     (entryKind == EntryKind::AccelerationStructure || entryKind == EntryKind::PartitionedAccelerationStructure));
	if (!entryMatchesBinding)
	{
		SPDLOG_LOGGER_ERROR(
		    g_vulkanDescriptorAllocatorLogger,
		    "Refused Vulkan descriptor write for binding '{}' because entry kind does not match compiled semantic "
		    "(semantic='{}', entryKind={}).",
		    binding.Name != nullptr ? binding.Name : "<unnamed>",
		    VulkanDescriptorAllocatorDiagnostics::ToString(binding.SemanticKind),
		    static_cast<std::uint32_t>(entryKind));
		return;
	}

	std::vector<VkDescriptorImageInfo> imageInfos;
	std::vector<VkDescriptorBufferInfo> bufferInfos;
	std::vector<VkAccelerationStructureKHR> accelerationStructures;
	std::vector<VkDeviceAddress> partitionedAccelerationStructureAddresses;
	imageInfos.reserve(entries.size());
	bufferInfos.reserve(entries.size());
	accelerationStructures.reserve(entries.size());
	partitionedAccelerationStructureAddresses.reserve(entries.size());
	for (const DescriptorEntry& entry : entries)
	{
		if (entry.Kind != entryKind)
		{
			return;
		}

		if (entryKind == EntryKind::SampledImage || entryKind == EntryKind::StorageImage || entryKind == EntryKind::Sampler)
		{
			imageInfos.push_back(entry.Image);
		}
		else if (entryKind == EntryKind::AccelerationStructure)
		{
			accelerationStructures.push_back(entry.AccelerationStructure);
		}
		else if (entryKind == EntryKind::PartitionedAccelerationStructure)
		{
			partitionedAccelerationStructureAddresses.push_back(entry.PartitionedAccelerationStructureAddress);
		}
		else
		{
			bufferInfos.push_back(entry.Buffer);
		}
	}

	const VkWriteDescriptorSetAccelerationStructureKHR accelerationStructureWrite{
	    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
	    .pNext = nullptr,
	    .accelerationStructureCount = static_cast<std::uint32_t>(accelerationStructures.size()),
	    .pAccelerationStructures = accelerationStructures.empty() ? nullptr : accelerationStructures.data()};
	const VkWriteDescriptorSetPartitionedAccelerationStructureNV partitionedAccelerationStructureWrite{
	    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_PARTITIONED_ACCELERATION_STRUCTURE_NV,
	    .pNext = nullptr,
	    .accelerationStructureCount = static_cast<std::uint32_t>(partitionedAccelerationStructureAddresses.size()),
	    .pAccelerationStructures =
	        partitionedAccelerationStructureAddresses.empty() ? nullptr : partitionedAccelerationStructureAddresses.data()};
	const VkWriteDescriptorSet write{
	    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	    .pNext = entryKind == EntryKind::AccelerationStructure
	                 ? static_cast<const void*>(&accelerationStructureWrite)
	                 : (entryKind == EntryKind::PartitionedAccelerationStructure
	                        ? static_cast<const void*>(&partitionedAccelerationStructureWrite)
	                        : nullptr),
	    .dstSet = descriptorSet,
	    .dstBinding = binding.BindingPoint.Binding,
	    .dstArrayElement = 0,
	    .descriptorCount = static_cast<std::uint32_t>(entries.size()),
	    .descriptorType = descriptorType,
	    .pImageInfo = imageInfos.empty() ? nullptr : imageInfos.data(),
	    .pBufferInfo = bufferInfos.empty() ? nullptr : bufferInfos.data(),
	    .pTexelBufferView = nullptr};
	vkUpdateDescriptorSets(m_rhi.GetDevice(), 1, &write, 0, nullptr);
}
