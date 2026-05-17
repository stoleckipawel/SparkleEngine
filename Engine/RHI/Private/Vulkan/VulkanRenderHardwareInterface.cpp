#include "Vulkan/VulkanPCH.h"

#include "Vulkan/VulkanRenderHardwareInterface.h"

#include "Config/RenderConfig.h"
#include "Shaders/CookedShaderPackage.h"
#include "Vulkan/Commands/VulkanCommandContext.h"
#include "Vulkan/Commands/VulkanRenderCommandList.h"
#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Descriptors/VulkanDescriptorAllocator.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Diagnostics/VulkanRenderDiagnostics.h"
#include "Vulkan/Memory/VulkanGpuAllocation.h"
#include "Vulkan/Memory/VulkanGpuMemoryAllocator.h"
#include "Vulkan/Pipeline/VulkanBindingLayout.h"
#include "Vulkan/Pipeline/VulkanPipelineState.h"
#include "Vulkan/SwapChain/VulkanSwapChain.h"
#include "Vulkan/VulkanTypeConversions.h"

#include <format>

static const auto g_vulkanRenderHardwareInterfaceLogger = Logging::GetOrCreateLogger("RHI.Vulkan.Interface");

VulkanRenderHardwareInterface::VulkanRenderHardwareInterface(
    VulkanRhi& rhi,
    VulkanSwapChain& swapChain,
    VulkanCommandContext& commandContext,
    VulkanGpuMemoryAllocator& memoryAllocator) noexcept :
    m_rhi(&rhi), m_swapChain(&swapChain), m_commandContext(&commandContext), m_memoryAllocator(&memoryAllocator)
{
	m_descriptorAllocator = std::make_unique<VulkanDescriptorAllocator>(rhi);
	m_uniformUploadRecordsByFrame.resize(RenderConfig::FramesInFlight);
	for (std::uint32_t frameIndex = 0; frameIndex < RenderConfig::FramesInFlight; ++frameIndex)
	{
		commandContext.GetCommandList(frameIndex).SetMemoryAllocator(&memoryAllocator);
		commandContext.GetCommandList(frameIndex).SetDescriptorAllocator(m_descriptorAllocator.get());
	}
	m_diagnostics = CreateVulkanRenderDiagnostics(rhi, memoryAllocator);
	RebuildSwapChainBackBufferViews();
}

VulkanRenderHardwareInterface::~VulkanRenderHardwareInterface() noexcept
{
	ReleaseAllResourceViews();
	for (SamplerRecord& samplerRecord : m_samplerRecords)
	{
		if (m_descriptorAllocator != nullptr && samplerRecord.Table)
		{
			m_descriptorAllocator->ReleaseDescriptorTable(samplerRecord.Table);
		}
		if (m_rhi != nullptr && samplerRecord.Sampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(m_rhi->GetDevice(), samplerRecord.Sampler, nullptr);
		}
	}
	m_samplerRecords.clear();
	m_uniformUploadRecordsByFrame.clear();
	if (m_memoryAllocator != nullptr)
	{
		m_memoryAllocator->FlushPendingReleases();
	}
}

ERhiBackendApi VulkanRenderHardwareInterface::GetBackendApi() const noexcept
{
	return ERhiBackendApi::Vulkan;
}

CookedShaderBinaryFormat VulkanRenderHardwareInterface::GetRequiredShaderBinaryFormat() const noexcept
{
	return CookedShaderBinaryFormat::SpirV;
}

std::uint32_t VulkanRenderHardwareInterface::GetCurrentFrameIndex() const noexcept
{
	return m_currentFrameIndex;
}

void VulkanRenderHardwareInterface::WaitForIdle() noexcept
{
	if (m_commandContext != nullptr)
	{
		m_commandContext->WaitForIdle();
	}
	if (m_rhi != nullptr)
	{
		m_rhi->WaitForIdle();
	}
	if (m_memoryAllocator != nullptr)
	{
		m_memoryAllocator->FlushPendingReleases();
	}
}

NativeGraphicsDeviceHandle VulkanRenderHardwareInterface::GetDeviceHandle() const noexcept
{
	return NativeGraphicsDeviceHandle{m_rhi != nullptr ? m_rhi->GetDevice() : nullptr};
}

NativeGraphicsQueueHandle VulkanRenderHardwareInterface::GetGraphicsQueueHandle() const noexcept
{
	return NativeGraphicsQueueHandle{m_rhi != nullptr ? m_rhi->GetGraphicsQueue() : nullptr};
}

RenderCommandList& VulkanRenderHardwareInterface::GetGraphicsCommandList(std::uint32_t) noexcept
{
	return m_commandContext->GetCommandList(m_currentFrameIndex);
}

RhiRayTracingCapabilities VulkanRenderHardwareInterface::GetRayTracingCapabilities() const noexcept
{
	return m_rhi != nullptr ? m_rhi->GetRayTracingCapabilities() : RhiRayTracingCapabilities{};
}

RenderDiagnostics& VulkanRenderHardwareInterface::GetDiagnostics() noexcept
{
	return *m_diagnostics;
}

const RenderDiagnostics& VulkanRenderHardwareInterface::GetDiagnostics() const noexcept
{
	return *m_diagnostics;
}

bool VulkanRenderHardwareInterface::InitializeImGuiBackend()
{
	return false;
}

void VulkanRenderHardwareInterface::BeginImGuiFrame() noexcept {}

void VulkanRenderHardwareInterface::RenderImGuiDrawData(ImDrawData*) noexcept {}

void VulkanRenderHardwareInterface::ShutdownImGuiBackend() noexcept {}

std::unique_ptr<RenderBindingLayout> VulkanRenderHardwareInterface::CreateBindingLayout(const RenderBindingLayoutCompileDesc& desc)
{
	return VulkanBindingLayoutCompiler::Compile(*m_rhi, desc);
}

std::unique_ptr<RenderPipelineState> VulkanRenderHardwareInterface::CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc)
{
	return std::make_unique<VulkanPipelineState>(*m_rhi, desc);
}

std::unique_ptr<RenderPipelineState> VulkanRenderHardwareInterface::CreateComputePipelineState(const ComputePipelineStateDesc& desc)
{
	return std::make_unique<VulkanPipelineState>(*m_rhi, desc);
}

void VulkanRenderHardwareInterface::BindGlobalDescriptorState(RenderCommandList&) const noexcept {}

RhiDescriptorAllocation VulkanRenderHardwareInterface::AllocateDescriptor(ERhiDescriptorAllocatorType descriptorType)
{
	return m_descriptorAllocator != nullptr ? m_descriptorAllocator->AllocateDescriptor(descriptorType) : RhiDescriptorAllocation{};
}

void VulkanRenderHardwareInterface::ReleaseDescriptor(
    ERhiDescriptorAllocatorType descriptorType,
    const RhiDescriptorAllocation& allocation) noexcept
{
	if (m_descriptorAllocator != nullptr)
	{
		m_descriptorAllocator->ReleaseDescriptor(descriptorType, allocation);
	}
}

RhiDescriptorTableHandle VulkanRenderHardwareInterface::AllocateDescriptorTable(
    ERhiDescriptorAllocatorType descriptorType,
    std::uint32_t descriptorCount)
{
	return m_descriptorAllocator != nullptr ? m_descriptorAllocator->AllocateDescriptorTable(descriptorType, descriptorCount)
	                                        : RhiDescriptorTableHandle{};
}

RhiCpuDescriptorHandle VulkanRenderHardwareInterface::GetDescriptorTableCpuHandle(
    RhiDescriptorTableHandle tableHandle,
    std::uint32_t descriptorIndex) const noexcept
{
	return m_descriptorAllocator != nullptr ? m_descriptorAllocator->GetDescriptorTableCpuHandle(tableHandle, descriptorIndex)
	                                        : RhiCpuDescriptorHandle{};
}

void VulkanRenderHardwareInterface::ReleaseDescriptorTable(RhiDescriptorTableHandle tableHandle) noexcept
{
	if (m_descriptorAllocator != nullptr)
	{
		m_descriptorAllocator->ReleaseDescriptorTable(tableHandle);
	}
}

void VulkanRenderHardwareInterface::AllocateShaderResourceDescriptor(
    RhiCpuDescriptorHandle& outCpuHandle,
    RhiGpuDescriptorHandle& outGpuHandle)
{
	const RhiDescriptorAllocation allocation = AllocateDescriptor(ERhiDescriptorAllocatorType::ShaderResource);
	outCpuHandle = allocation.CpuHandle;
	outGpuHandle = allocation.GpuHandle;
}

void VulkanRenderHardwareInterface::ReleaseShaderResourceDescriptor(
    RhiCpuDescriptorHandle cpuHandle,
    RhiGpuDescriptorHandle gpuHandle) noexcept
{
	ReleaseDescriptor(ERhiDescriptorAllocatorType::ShaderResource, RhiDescriptorAllocation{.CpuHandle = cpuHandle, .GpuHandle = gpuHandle});
}

const PerFrameConstantBufferData& VulkanRenderHardwareInterface::GetPerFrameConstantData() const noexcept
{
	return m_emptyPerFrameConstants;
}

RhiGpuVirtualAddress VulkanRenderHardwareInterface::GetPerFrameConstantGpuAddress() const noexcept
{
	return {};
}

RhiGpuVirtualAddress VulkanRenderHardwareInterface::AllocateUniformConstantBuffer(const void* data, std::uint32_t sizeInBytes)
{
	if (m_memoryAllocator == nullptr || data == nullptr || sizeInBytes == 0)
	{
		return {};
	}

	const RhiBufferResourceDesc desc{.SizeInBytes = sizeInBytes, .StrideInBytes = 0, .AllowUnorderedAccess = false};
	const VkBufferCreateInfo createInfo = VulkanTypeConversions::BuildBufferCreateInfo(desc, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	std::unique_ptr<VulkanGpuAllocationRecord> record = m_memoryAllocator->CreateBuffer(
	    createInfo,
	    RhiMemoryCategory::ConstantBuffer,
	    RhiMemoryResidencyClass::HostUpload,
	    L"Vulkan Uniform Upload");
	if (record == nullptr || record->Buffer == VK_NULL_HANDLE || !m_memoryAllocator->WriteAllocation(*record, data, sizeInBytes))
	{
		return {};
	}

	const VkBuffer buffer = record->Buffer;
	if (m_currentFrameIndex < m_uniformUploadRecordsByFrame.size())
	{
		m_uniformUploadRecordsByFrame[m_currentFrameIndex].push_back(std::move(record));
	}
	return reinterpret_cast<RhiGpuVirtualAddress>(buffer);
}

RhiGpuVirtualAddress VulkanRenderHardwareInterface::AllocatePerViewConstantBuffer(const PerViewConstantBufferData& data)
{
	return AllocateUniformConstantBuffer(&data, sizeof(data));
}

RhiGpuVirtualAddress VulkanRenderHardwareInterface::AllocatePerObjectVertexConstants(const PerObjectVSConstantBufferData& data)
{
	return AllocateUniformConstantBuffer(&data, sizeof(data));
}

RhiGpuVirtualAddress VulkanRenderHardwareInterface::AllocatePerObjectPixelConstants(const PerObjectPSConstantBufferData& data)
{
	return AllocateUniformConstantBuffer(&data, sizeof(data));
}

RhiDescriptorTableBinding VulkanRenderHardwareInterface::GetSharedSamplerBinding(const RhiSamplerDesc& samplerDesc) const noexcept
{
	if (m_descriptorAllocator == nullptr || m_rhi == nullptr)
	{
		return {};
	}

	for (const SamplerRecord& samplerRecord : m_samplerRecords)
	{
		if (SamplerDescEquals(samplerRecord.Desc, samplerDesc))
		{
			return RhiDescriptorTableBinding{.Table = samplerRecord.Table, .DescriptorIndex = 0};
		}
	}

	VulkanRenderHardwareInterface* self = const_cast<VulkanRenderHardwareInterface*>(this);
	VkSampler sampler = self->CreateSampler(samplerDesc);
	if (sampler == VK_NULL_HANDLE)
	{
		return {};
	}
	const RhiDescriptorTableHandle table = self->m_descriptorAllocator->AllocateDescriptorTable(ERhiDescriptorAllocatorType::Sampler, 1);
	self->m_descriptorAllocator->WriteSamplerDescriptor(self->m_descriptorAllocator->GetDescriptorTableCpuHandle(table), sampler);
	self->m_samplerRecords.push_back(SamplerRecord{.Desc = samplerDesc, .Sampler = sampler, .Table = table});
	return RhiDescriptorTableBinding{.Table = table, .DescriptorIndex = 0};
}

RhiViewport VulkanRenderHardwareInterface::GetBackBufferViewport() const noexcept
{
	return m_swapChain != nullptr ? m_swapChain->GetDefaultViewport() : RhiViewport{};
}

RhiRect VulkanRenderHardwareInterface::GetBackBufferScissorRect() const noexcept
{
	return m_swapChain != nullptr ? m_swapChain->GetDefaultScissorRect() : RhiRect{};
}

RhiCpuDescriptorHandle VulkanRenderHardwareInterface::GetBackBufferRenderTargetView() const noexcept
{
	return GetResourceViewCpuHandle(GetCurrentBackBufferViewHandle());
}

NativeResourceHandle VulkanRenderHardwareInterface::GetBackBufferResource() const noexcept
{
	return m_swapChain != nullptr ? m_swapChain->GetCurrentBackBufferResource() : NativeResourceHandle{};
}

std::unique_ptr<Texture> VulkanRenderHardwareInterface::CreateTextureFromPath(const std::filesystem::path&) const
{
	FailRenderingNotImplemented("CreateTextureFromPath");
	return {};
}

RhiOwnedResourceHandle VulkanRenderHardwareInterface::CreateTextureResource(
    const RhiTextureResourceDesc& desc,
    ResourceState initialState,
    RhiMemoryCategory category,
    RhiMemoryResidencyClass residencyClass,
    std::wstring_view debugName)
{
	(void) initialState;
	if (m_memoryAllocator == nullptr || desc.Width == 0 || desc.Height == 0 || desc.Format == PixelFormat::Unknown)
	{
		return {};
	}

	const VkImageCreateInfo imageCreateInfo = VulkanTypeConversions::BuildTextureCreateInfo(desc);
	std::unique_ptr<VulkanGpuAllocationRecord> record =
	    m_memoryAllocator->CreateImage(imageCreateInfo, category, residencyClass, debugName);
	return record != nullptr ? MakeVulkanOwnedResourceHandle(std::move(record)) : RhiOwnedResourceHandle{};
}

RhiOwnedResourceHandle VulkanRenderHardwareInterface::CreateBufferResource(
    const RhiBufferResourceDesc& desc,
    ResourceState initialState,
    RhiMemoryCategory category,
    RhiMemoryResidencyClass residencyClass,
    std::wstring_view debugName)
{
	(void) initialState;
	if (m_memoryAllocator == nullptr || desc.SizeInBytes == 0)
	{
		return {};
	}

	const VkBufferCreateInfo bufferCreateInfo = VulkanTypeConversions::BuildBufferCreateInfo(desc);
	std::unique_ptr<VulkanGpuAllocationRecord> record =
	    m_memoryAllocator->CreateBuffer(bufferCreateInfo, category, residencyClass, debugName);
	return record != nullptr ? MakeVulkanOwnedResourceHandle(std::move(record)) : RhiOwnedResourceHandle{};
}

bool VulkanRenderHardwareInterface::CreateVertexBuffer(
    const void* data,
    std::size_t sizeInBytes,
    std::uint32_t strideInBytes,
    std::wstring_view debugName,
    RhiOwnedResourceHandle& outResource,
    RhiVertexBufferView& outView)
{
	outResource = {};
	outView = {};
	if (m_memoryAllocator == nullptr || data == nullptr || sizeInBytes == 0 || strideInBytes == 0)
	{
		return false;
	}

	const RhiBufferResourceDesc desc{.SizeInBytes = sizeInBytes, .StrideInBytes = strideInBytes};
	const VkBufferCreateInfo bufferCreateInfo = VulkanTypeConversions::BuildBufferCreateInfo(desc, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	std::unique_ptr<VulkanGpuAllocationRecord> record = m_memoryAllocator->CreateBuffer(
	    bufferCreateInfo,
	    RhiMemoryCategory::Mesh,
	    RhiMemoryResidencyClass::HostUpload,
	    debugName.empty() ? L"VertexBuffer" : debugName);
	if (record == nullptr || record->Buffer == VK_NULL_HANDLE || !m_memoryAllocator->WriteAllocation(*record, data, sizeInBytes))
	{
		return false;
	}

	outView = RhiVertexBufferView{
	    .BufferLocation = reinterpret_cast<std::uint64_t>(record->Buffer),
	    .SizeInBytes = static_cast<std::uint32_t>(sizeInBytes),
	    .StrideInBytes = strideInBytes};
	outResource = MakeVulkanOwnedResourceHandle(std::move(record));
	return true;
}

bool VulkanRenderHardwareInterface::CreateIndexBuffer(
    const void* data,
    std::size_t sizeInBytes,
    RhiIndexFormat format,
    std::wstring_view debugName,
    RhiOwnedResourceHandle& outResource,
    RhiIndexBufferView& outView)
{
	outResource = {};
	outView = {};
	if (m_memoryAllocator == nullptr || data == nullptr || sizeInBytes == 0)
	{
		return false;
	}

	const RhiBufferResourceDesc desc{.SizeInBytes = sizeInBytes};
	const VkBufferCreateInfo bufferCreateInfo = VulkanTypeConversions::BuildBufferCreateInfo(desc, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	std::unique_ptr<VulkanGpuAllocationRecord> record = m_memoryAllocator->CreateBuffer(
	    bufferCreateInfo,
	    RhiMemoryCategory::Mesh,
	    RhiMemoryResidencyClass::HostUpload,
	    debugName.empty() ? L"IndexBuffer" : debugName);
	if (record == nullptr || record->Buffer == VK_NULL_HANDLE || !m_memoryAllocator->WriteAllocation(*record, data, sizeInBytes))
	{
		return false;
	}

	outView = RhiIndexBufferView{
	    .BufferLocation = reinterpret_cast<std::uint64_t>(record->Buffer),
	    .SizeInBytes = static_cast<std::uint32_t>(sizeInBytes),
	    .Format = format};
	outResource = MakeVulkanOwnedResourceHandle(std::move(record));
	return true;
}

void VulkanRenderHardwareInterface::ReleaseOwnedResource(RhiOwnedResourceHandle resource) noexcept
{
	if (m_memoryAllocator == nullptr)
	{
		return;
	}

	std::unique_ptr<VulkanGpuAllocationRecord> record = TakeVulkanOwnedResourceHandle(resource);
	if (record == nullptr)
	{
		return;
	}

	const std::uint64_t retireFenceValue = m_commandContext != nullptr ? m_commandContext->GetNextRetireFenceValue() : 0;
	m_memoryAllocator->QueueDestroyResource(std::move(record), retireFenceValue);
	if (m_commandContext != nullptr)
	{
		m_memoryAllocator->DrainCompletedReleases(m_commandContext->GetCompletedRetireFenceValue());
	}
	else
	{
		m_memoryAllocator->FlushPendingReleases();
	}
}

NativeResourceHandle VulkanRenderHardwareInterface::GetNativeResource(RhiOwnedResourceHandle resource) const noexcept
{
	VulkanGpuAllocationRecord* const record = GetVulkanGpuAllocationRecord(resource);
	return record != nullptr ? GetVulkanNativeResource(*record) : NativeResourceHandle{};
}

RhiGpuVirtualAddress VulkanRenderHardwareInterface::GetResourceGpuVirtualAddress(RhiOwnedResourceHandle resource) const noexcept
{
	VulkanGpuAllocationRecord* const record = GetVulkanGpuAllocationRecord(resource);
	return record != nullptr && record->Buffer != VK_NULL_HANDLE ? reinterpret_cast<std::uint64_t>(record->Buffer) : 0;
}

RhiRayTracingAccelerationStructurePrebuildInfo VulkanRenderHardwareInterface::GetBottomLevelAccelerationStructurePrebuildInfo(
    const RhiRayTracingGeometryDesc&) const noexcept
{
	return {};
}

RhiRayTracingAccelerationStructurePrebuildInfo VulkanRenderHardwareInterface::GetTopLevelAccelerationStructurePrebuildInfo(
    std::uint32_t) const noexcept
{
	return {};
}

RhiOwnedResourceHandle VulkanRenderHardwareInterface::CreateRayTracingScratchBuffer(std::uint64_t, std::wstring_view)
{
	FailRenderingNotImplemented("CreateRayTracingScratchBuffer");
	return {};
}

RhiOwnedResourceHandle VulkanRenderHardwareInterface::CreateRayTracingAccelerationStructureBuffer(std::uint64_t, std::wstring_view)
{
	FailRenderingNotImplemented("CreateRayTracingAccelerationStructureBuffer");
	return {};
}

RhiOwnedResourceHandle VulkanRenderHardwareInterface::CreateRayTracingInstanceBuffer(
    const RhiRayTracingInstanceDesc*,
    std::uint32_t,
    std::wstring_view)
{
	FailRenderingNotImplemented("CreateRayTracingInstanceBuffer");
	return {};
}

RhiResourceAllocationInfo VulkanRenderHardwareInterface::GetTextureAllocationInfo(const RhiTextureResourceDesc& desc) const noexcept
{
	if (m_rhi == nullptr || desc.Width == 0 || desc.Height == 0 || desc.Format == PixelFormat::Unknown)
	{
		return {};
	}

	const VkImageCreateInfo imageCreateInfo = VulkanTypeConversions::BuildTextureCreateInfo(desc);
	const VkDeviceImageMemoryRequirements requirementsInfo{
	    .sType = VK_STRUCTURE_TYPE_DEVICE_IMAGE_MEMORY_REQUIREMENTS,
	    .pNext = nullptr,
	    .pCreateInfo = &imageCreateInfo,
	    .planeAspect = static_cast<VkImageAspectFlagBits>(0)};
	VkMemoryRequirements2 memoryRequirements{.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
	vkGetDeviceImageMemoryRequirements(m_rhi->GetDevice(), &requirementsInfo, &memoryRequirements);
	return RhiResourceAllocationInfo{
	    .SizeInBytes = memoryRequirements.memoryRequirements.size,
	    .Alignment = memoryRequirements.memoryRequirements.alignment};
}

RhiResourceAllocationInfo VulkanRenderHardwareInterface::GetBufferAllocationInfo(const RhiBufferResourceDesc& desc) const noexcept
{
	if (m_rhi == nullptr || desc.SizeInBytes == 0)
	{
		return {};
	}

	const VkBufferCreateInfo bufferCreateInfo = VulkanTypeConversions::BuildBufferCreateInfo(desc);
	const VkDeviceBufferMemoryRequirements requirementsInfo{
	    .sType = VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS,
	    .pNext = nullptr,
	    .pCreateInfo = &bufferCreateInfo};
	VkMemoryRequirements2 memoryRequirements{.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
	vkGetDeviceBufferMemoryRequirements(m_rhi->GetDevice(), &requirementsInfo, &memoryRequirements);
	return RhiResourceAllocationInfo{
	    .SizeInBytes = memoryRequirements.memoryRequirements.size,
	    .Alignment = memoryRequirements.memoryRequirements.alignment};
}

RhiOwnedMemoryBlockHandle VulkanRenderHardwareInterface::CreateTransientMemoryBlock(
    RhiTransientAllocationPool pool,
    std::uint64_t sizeInBytes,
    std::uint64_t alignment,
    std::wstring_view debugName)
{
	if (m_memoryAllocator == nullptr || sizeInBytes == 0)
	{
		return {};
	}

	std::unique_ptr<VulkanGpuMemoryBlockRecord> record =
	    m_memoryAllocator->CreateTransientMemoryBlock(pool, sizeInBytes, alignment, debugName);
	return record != nullptr ? MakeVulkanOwnedMemoryBlockHandle(std::move(record)) : RhiOwnedMemoryBlockHandle{};
}

void VulkanRenderHardwareInterface::ReleaseTransientMemoryBlock(RhiOwnedMemoryBlockHandle memoryBlock) noexcept
{
	if (m_memoryAllocator == nullptr)
	{
		return;
	}

	std::unique_ptr<VulkanGpuMemoryBlockRecord> record = TakeVulkanOwnedMemoryBlockHandle(memoryBlock);
	if (record == nullptr)
	{
		return;
	}

	const std::uint64_t retireFenceValue = m_commandContext != nullptr ? m_commandContext->GetNextRetireFenceValue() : 0;
	m_memoryAllocator->QueueDestroyMemoryBlock(std::move(record), retireFenceValue);
	if (m_commandContext != nullptr)
	{
		m_memoryAllocator->DrainCompletedReleases(m_commandContext->GetCompletedRetireFenceValue());
	}
	else
	{
		m_memoryAllocator->FlushPendingReleases();
	}
}

RhiOwnedResourceHandle VulkanRenderHardwareInterface::CreateAliasingTextureResource(
    RhiOwnedMemoryBlockHandle memoryBlock,
    std::uint64_t memoryBlockOffset,
    const RhiTransientTextureAllocationDesc& desc,
    std::wstring_view debugName)
{
	(void) desc.InitialState;
	(void) desc.ClearValue;
	if (m_memoryAllocator == nullptr || !memoryBlock || desc.ResourceDesc.Width == 0 || desc.ResourceDesc.Height == 0 ||
	    desc.ResourceDesc.Format == PixelFormat::Unknown)
	{
		return {};
	}

	VulkanGpuMemoryBlockRecord* const memoryBlockRecord = GetVulkanGpuMemoryBlockRecord(memoryBlock);
	if (memoryBlockRecord == nullptr)
	{
		return {};
	}

	const VkImageCreateInfo imageCreateInfo = VulkanTypeConversions::BuildTextureCreateInfo(desc.ResourceDesc);
	std::unique_ptr<VulkanGpuAllocationRecord> record =
	    m_memoryAllocator->CreateAliasingImage(*memoryBlockRecord, memoryBlockOffset, imageCreateInfo, debugName);
	return record != nullptr ? MakeVulkanOwnedResourceHandle(std::move(record)) : RhiOwnedResourceHandle{};
}

RhiOwnedResourceHandle VulkanRenderHardwareInterface::CreateAliasingBufferResource(
    RhiOwnedMemoryBlockHandle memoryBlock,
    std::uint64_t memoryBlockOffset,
    const RhiTransientBufferAllocationDesc& desc,
    std::wstring_view debugName)
{
	(void) desc.InitialState;
	if (m_memoryAllocator == nullptr || !memoryBlock || desc.ResourceDesc.SizeInBytes == 0)
	{
		return {};
	}

	VulkanGpuMemoryBlockRecord* const memoryBlockRecord = GetVulkanGpuMemoryBlockRecord(memoryBlock);
	if (memoryBlockRecord == nullptr)
	{
		return {};
	}

	const VkBufferCreateInfo bufferCreateInfo = VulkanTypeConversions::BuildBufferCreateInfo(desc.ResourceDesc);
	std::unique_ptr<VulkanGpuAllocationRecord> record =
	    m_memoryAllocator->CreateAliasingBuffer(*memoryBlockRecord, memoryBlockOffset, bufferCreateInfo, debugName);
	return record != nullptr ? MakeVulkanOwnedResourceHandle(std::move(record)) : RhiOwnedResourceHandle{};
}

RhiResourceViewHandle VulkanRenderHardwareInterface::CreateResourceView(const RhiResourceViewDesc& desc)
{
	if (!desc.Resource)
	{
		return {};
	}

	switch (desc.Kind)
	{
		case ERhiResourceViewKind::TextureShaderResource:
		case ERhiResourceViewKind::TextureUnorderedAccess:
		{
			const VkImageView imageView = CreateImageView(desc);
			RhiGpuDescriptorHandle descriptorHandle = {};
			if (m_descriptorAllocator != nullptr && imageView != VK_NULL_HANDLE)
			{
				descriptorHandle = m_descriptorAllocator->RegisterImageDescriptor(desc.Kind, imageView);
			}
			return AddResourceView(
			    ResourceViewRecord{
			        .Kind = desc.Kind,
			        .Image = static_cast<VkImage>(desc.Resource.Value),
			        .ImageView = imageView,
			        .DescriptorHandle = descriptorHandle,
			        .OwnsImageView = true});
		}
		case ERhiResourceViewKind::RenderTarget:
		case ERhiResourceViewKind::DepthStencil:
			return AddResourceView(
			    ResourceViewRecord{
			        .Kind = desc.Kind,
			        .Image = static_cast<VkImage>(desc.Resource.Value),
			        .ImageView = CreateImageView(desc),
			        .OwnsImageView = true});
		case ERhiResourceViewKind::BufferShaderResource:
		case ERhiResourceViewKind::BufferUnorderedAccess:
		{
			RhiGpuDescriptorHandle descriptorHandle = {};
			if (m_descriptorAllocator != nullptr)
			{
				descriptorHandle = m_descriptorAllocator->RegisterBufferDescriptor(
				    static_cast<VkBuffer>(desc.Resource.Value),
				    desc.Buffer.OffsetInBytes,
				    desc.Buffer.SizeInBytes != 0 ? desc.Buffer.SizeInBytes : VK_WHOLE_SIZE);
			}
			return AddResourceView(
			    ResourceViewRecord{
			        .Kind = desc.Kind,
			        .Buffer = static_cast<VkBuffer>(desc.Resource.Value),
			        .DescriptorHandle = descriptorHandle});
		}
		case ERhiResourceViewKind::AccelerationStructureShaderResource:
		default:
			FailRenderingNotImplemented("CreateResourceView for non-texture Vulkan resources");
			return {};
	}
}

void VulkanRenderHardwareInterface::ReleaseResourceView(RhiResourceViewHandle view) noexcept
{
	ResourceViewRecord* const record = FindResourceViewRecord(view);
	if (record == nullptr)
	{
		return;
	}

	if (record->OwnsImageView && record->ImageView != VK_NULL_HANDLE && m_rhi != nullptr)
	{
		vkDestroyImageView(m_rhi->GetDevice(), record->ImageView, nullptr);
	}
	if (m_descriptorAllocator != nullptr && record->DescriptorHandle)
	{
		m_descriptorAllocator->ReleaseRegisteredDescriptor(record->DescriptorHandle);
	}
	*record = {};
	m_freeResourceViewIndices.push_back(view.Value - 1u);
}

RhiCpuDescriptorHandle VulkanRenderHardwareInterface::GetResourceViewCpuHandle(RhiResourceViewHandle view) const noexcept
{
	const ResourceViewRecord* const record = FindResourceViewRecord(view);
	return RhiCpuDescriptorHandle{record != nullptr ? EncodeImageViewHandle(record->ImageView) : 0u};
}

RhiGpuDescriptorHandle VulkanRenderHardwareInterface::GetResourceViewGpuHandle(RhiResourceViewHandle view) const noexcept
{
	const ResourceViewRecord* const record = FindResourceViewRecord(view);
	return record != nullptr ? record->DescriptorHandle : RhiGpuDescriptorHandle{};
}

bool VulkanRenderHardwareInterface::SupportsUnorderedAccess(NativeResourceHandle) const noexcept
{
	return false;
}

void VulkanRenderHardwareInterface::BeginPresentRenderPass(const float clearColor[4]) noexcept
{
	static constexpr float defaultClearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	BeginCurrentBackBufferRendering(clearColor != nullptr ? clearColor : defaultClearColor, true);
}

void VulkanRenderHardwareInterface::BeginPresentOverlayPass() noexcept
{
	BeginCurrentBackBufferRendering(nullptr, false);
}

void VulkanRenderHardwareInterface::EndPresentRenderPass() noexcept
{
	EndCurrentBackBufferRendering();
}

PixelFormat VulkanRenderHardwareInterface::GetPresentColorFormat() const noexcept
{
	return m_swapChain != nullptr ? m_swapChain->GetBackBufferFormat() : PixelFormat::Unknown;
}

void VulkanRenderHardwareInterface::SetCurrentFrameIndex(std::uint32_t frameIndex) noexcept
{
	m_currentFrameIndex = frameIndex;
}

void VulkanRenderHardwareInterface::ResetTransientFrameResources() noexcept
{
	if (m_descriptorAllocator != nullptr)
	{
		m_descriptorAllocator->BeginFrame(m_currentFrameIndex);
	}
	if (m_currentFrameIndex < m_uniformUploadRecordsByFrame.size())
	{
		m_uniformUploadRecordsByFrame[m_currentFrameIndex].clear();
	}
}

void VulkanRenderHardwareInterface::RebuildSwapChainBackBufferViews() noexcept
{
	ReleaseAllResourceViews();
	m_resourceViewRecords.clear();
	m_freeResourceViewIndices.clear();
	m_swapChainBackBufferViews.clear();
	m_swapChainBackBufferLayouts.clear();
	m_isPresentRendering = false;
	if (m_swapChain == nullptr)
	{
		return;
	}

	const std::uint32_t backBufferCount = m_swapChain->GetBackBufferCount();
	m_swapChainBackBufferViews.reserve(backBufferCount);
	m_swapChainBackBufferLayouts.assign(backBufferCount, VK_IMAGE_LAYOUT_UNDEFINED);
	for (std::uint32_t backBufferIndex = 0; backBufferIndex < backBufferCount; ++backBufferIndex)
	{
		m_swapChainBackBufferViews.push_back(AddResourceView(
		    ResourceViewRecord{
		        .Kind = ERhiResourceViewKind::RenderTarget,
		        .Image = m_swapChain->GetBackBufferImage(backBufferIndex),
		        .ImageView = m_swapChain->GetBackBufferImageView(backBufferIndex),
		        .OwnsImageView = false}));
	}
}

void VulkanRenderHardwareInterface::FailRenderingNotImplemented(std::string_view operation) noexcept
{
	Diagnostics::Fail(
	    g_vulkanRenderHardwareInterfaceLogger,
	    __FILE__,
	    __LINE__,
	    std::format(
	        "Vulkan RHI operation '{}' is not implemented before the resource, command, swapchain, and pipeline phases.",
	        operation));
}

RhiResourceViewHandle VulkanRenderHardwareInterface::MakeResourceViewHandle(std::uint32_t index) noexcept
{
	return RhiResourceViewHandle{index + 1u};
}

bool VulkanRenderHardwareInterface::SamplerDescEquals(const RhiSamplerDesc& lhs, const RhiSamplerDesc& rhs) noexcept
{
	return lhs.MinMagFilter == rhs.MinMagFilter && lhs.MipFilter == rhs.MipFilter && lhs.Address.U == rhs.Address.U &&
	       lhs.Address.V == rhs.Address.V && lhs.Address.W == rhs.Address.W && lhs.MaxAnisotropy == rhs.MaxAnisotropy;
}

VkFilter VulkanRenderHardwareInterface::ToVkFilter(RhiSamplerMinMagFilter filter) noexcept
{
	return filter == RhiSamplerMinMagFilter::Point ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
}

VkSamplerMipmapMode VulkanRenderHardwareInterface::ToVkMipmapMode(RhiSamplerMipFilter filter) noexcept
{
	return filter == RhiSamplerMipFilter::Point ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;
}

VkSamplerAddressMode VulkanRenderHardwareInterface::ToVkAddressMode(RhiSamplerAddressMode mode) noexcept
{
	switch (mode)
	{
		case RhiSamplerAddressMode::Clamp:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case RhiSamplerAddressMode::Mirror:
			return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		case RhiSamplerAddressMode::Wrap:
		default:
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}
}

std::uintptr_t VulkanRenderHardwareInterface::EncodeImageViewHandle(VkImageView imageView) noexcept
{
	return reinterpret_cast<std::uintptr_t>(imageView);
}

VkSampler VulkanRenderHardwareInterface::CreateSampler(const RhiSamplerDesc& desc) const
{
	const float maxAnisotropy = static_cast<float>(desc.MaxAnisotropy);
	const VkSamplerCreateInfo createInfo{
	    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .magFilter = ToVkFilter(desc.MinMagFilter),
	    .minFilter = ToVkFilter(desc.MinMagFilter),
	    .mipmapMode = ToVkMipmapMode(desc.MipFilter),
	    .addressModeU = ToVkAddressMode(desc.Address.U),
	    .addressModeV = ToVkAddressMode(desc.Address.V),
	    .addressModeW = ToVkAddressMode(desc.Address.W),
	    .mipLodBias = 0.0f,
	    .anisotropyEnable = desc.MaxAnisotropy != RhiSamplerAnisotropy::X1,
	    .maxAnisotropy = maxAnisotropy,
	    .compareEnable = VK_FALSE,
	    .compareOp = VK_COMPARE_OP_ALWAYS,
	    .minLod = 0.0f,
	    .maxLod = desc.MipFilter == RhiSamplerMipFilter::None ? 0.0f : VK_LOD_CLAMP_NONE,
	    .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
	    .unnormalizedCoordinates = VK_FALSE};

	VkSampler sampler = VK_NULL_HANDLE;
	const VkResult result = vkCreateSampler(m_rhi->GetDevice(), &createInfo, nullptr, &sampler);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fail(
		    g_vulkanRenderHardwareInterfaceLogger,
		    __FILE__,
		    __LINE__,
		    VulkanResult::FormatFailure("vkCreateSampler", result));
	}
	return sampler;
}

RhiResourceViewHandle VulkanRenderHardwareInterface::AddResourceView(ResourceViewRecord record)
{
	if (!m_freeResourceViewIndices.empty())
	{
		const std::uint32_t index = m_freeResourceViewIndices.back();
		m_freeResourceViewIndices.pop_back();
		m_resourceViewRecords[index] = record;
		return MakeResourceViewHandle(index);
	}

	m_resourceViewRecords.push_back(record);
	return MakeResourceViewHandle(static_cast<std::uint32_t>(m_resourceViewRecords.size() - 1u));
}

VulkanRenderHardwareInterface::ResourceViewRecord* VulkanRenderHardwareInterface::FindResourceViewRecord(
    RhiResourceViewHandle view) noexcept
{
	if (!view || view.Value - 1u >= m_resourceViewRecords.size())
	{
		return nullptr;
	}
	return &m_resourceViewRecords[view.Value - 1u];
}

const VulkanRenderHardwareInterface::ResourceViewRecord* VulkanRenderHardwareInterface::FindResourceViewRecord(
    RhiResourceViewHandle view) const noexcept
{
	if (!view || view.Value - 1u >= m_resourceViewRecords.size())
	{
		return nullptr;
	}
	return &m_resourceViewRecords[view.Value - 1u];
}

VkImageView VulkanRenderHardwareInterface::CreateImageView(const RhiResourceViewDesc& desc) const
{
	const VkImageViewCreateInfo createInfo{
	    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .image = static_cast<VkImage>(desc.Resource.Value),
	    .viewType = VK_IMAGE_VIEW_TYPE_2D,
	    .format = ResolveViewFormat(desc),
	    .components =
	        VkComponentMapping{
	            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
	            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
	            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
	            .a = VK_COMPONENT_SWIZZLE_IDENTITY},
	    .subresourceRange = VkImageSubresourceRange{
	        .aspectMask = ResolveViewAspectMask(desc),
	        .baseMipLevel = desc.Texture.MostDetailedMip,
	        .levelCount = desc.Texture.MipCount,
	        .baseArrayLayer = desc.Texture.FirstArraySlice,
	        .layerCount = desc.Texture.ArraySize}};

	VkImageView imageView = VK_NULL_HANDLE;
	const VkResult result = vkCreateImageView(m_rhi->GetDevice(), &createInfo, nullptr, &imageView);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fail(
		    g_vulkanRenderHardwareInterfaceLogger,
		    __FILE__,
		    __LINE__,
		    VulkanResult::FormatFailure("vkCreateImageView", result));
	}
	return imageView;
}

VkFormat VulkanRenderHardwareInterface::ResolveViewFormat(const RhiResourceViewDesc& desc) const noexcept
{
	if (desc.Format != PixelFormat::Unknown)
	{
		return VulkanTypeConversions::ToVkFormat(desc.Format);
	}
	if (m_swapChain != nullptr && desc.Resource.Value == m_swapChain->GetCurrentBackBufferResource().Value)
	{
		return m_swapChain->GetNativeBackBufferFormat();
	}
	return VK_FORMAT_UNDEFINED;
}

VkImageAspectFlags VulkanRenderHardwareInterface::ResolveViewAspectMask(const RhiResourceViewDesc& desc) const noexcept
{
	return desc.Kind == ERhiResourceViewKind::DepthStencil ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
	                                                       : VK_IMAGE_ASPECT_COLOR_BIT;
}

RhiResourceViewHandle VulkanRenderHardwareInterface::GetCurrentBackBufferViewHandle() const noexcept
{
	if (m_swapChain == nullptr)
	{
		return {};
	}

	const std::uint32_t backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
	return backBufferIndex < m_swapChainBackBufferViews.size() ? m_swapChainBackBufferViews[backBufferIndex] : RhiResourceViewHandle{};
}

void VulkanRenderHardwareInterface::ReleaseAllResourceViews() noexcept
{
	if (m_rhi == nullptr)
	{
		return;
	}

	for (ResourceViewRecord& record : m_resourceViewRecords)
	{
		if (record.OwnsImageView && record.ImageView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(m_rhi->GetDevice(), record.ImageView, nullptr);
		}
		if (m_descriptorAllocator != nullptr && record.DescriptorHandle)
		{
			m_descriptorAllocator->ReleaseRegisteredDescriptor(record.DescriptorHandle);
		}
		record = {};
	}
}

void VulkanRenderHardwareInterface::BeginCurrentBackBufferRendering(const float* clearColor, bool clear) noexcept
{
	if (m_swapChain == nullptr || m_commandContext == nullptr || m_isPresentRendering)
	{
		return;
	}

	const VkImage backBuffer = m_swapChain->GetCurrentBackBufferImage();
	const VkImageView backBufferView = m_swapChain->GetCurrentBackBufferImageView();
	if (backBuffer == VK_NULL_HANDLE || backBufferView == VK_NULL_HANDLE)
	{
		return;
	}

	RenderCommandList& commandList = GetGraphicsCommandList(m_currentFrameIndex);
	commandList.SetViewport(GetBackBufferViewport());
	commandList.SetScissorRect(GetBackBufferScissorRect());
	commandList.SetRenderTarget(GetBackBufferRenderTargetView());

	VkCommandBuffer commandBuffer = m_commandContext->GetCommandBuffer(m_currentFrameIndex);
	TransitionCurrentBackBuffer(commandBuffer, ResourceState::RenderTarget);

	VkClearValue nativeClearValue = {};
	if (clear && clearColor != nullptr)
	{
		nativeClearValue.color.float32[0] = clearColor[0];
		nativeClearValue.color.float32[1] = clearColor[1];
		nativeClearValue.color.float32[2] = clearColor[2];
		nativeClearValue.color.float32[3] = clearColor[3];
	}

	const RhiRect scissorRect = GetBackBufferScissorRect();
	const VkRenderingAttachmentInfo colorAttachment{
	    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
	    .pNext = nullptr,
	    .imageView = backBufferView,
	    .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	    .resolveMode = VK_RESOLVE_MODE_NONE,
	    .resolveImageView = VK_NULL_HANDLE,
	    .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	    .loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
	    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
	    .clearValue = nativeClearValue};

	const VkRenderingInfo renderingInfo{
	    .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .renderArea =
	        VkRect2D{
	            .offset = VkOffset2D{.x = scissorRect.Left, .y = scissorRect.Top},
	            .extent =
	                VkExtent2D{
	                    .width = static_cast<std::uint32_t>(scissorRect.Right - scissorRect.Left),
	                    .height = static_cast<std::uint32_t>(scissorRect.Bottom - scissorRect.Top)}},
	    .layerCount = 1,
	    .viewMask = 0,
	    .colorAttachmentCount = 1,
	    .pColorAttachments = &colorAttachment,
	    .pDepthAttachment = nullptr,
	    .pStencilAttachment = nullptr};

	vkCmdBeginRendering(commandBuffer, &renderingInfo);
	m_isPresentRendering = true;
}

void VulkanRenderHardwareInterface::EndCurrentBackBufferRendering() noexcept
{
	if (m_commandContext == nullptr || !m_isPresentRendering)
	{
		return;
	}

	VkCommandBuffer commandBuffer = m_commandContext->GetCommandBuffer(m_currentFrameIndex);
	vkCmdEndRendering(commandBuffer);
	TransitionCurrentBackBuffer(commandBuffer, ResourceState::Present);
	m_isPresentRendering = false;
}

void VulkanRenderHardwareInterface::TransitionCurrentBackBuffer(VkCommandBuffer commandBuffer, ResourceState newState) noexcept
{
	if (m_swapChain == nullptr || commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	const std::uint32_t backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
	if (backBufferIndex >= m_swapChainBackBufferLayouts.size())
	{
		return;
	}

	const VulkanResourceStateMapping destinationState = VulkanTypeConversions::ToResourceStateMapping(newState);
	const VkImageLayout newLayout = destinationState.ImageLayout;
	VkImageLayout& currentLayout = m_swapChainBackBufferLayouts[backBufferIndex];
	if (currentLayout == newLayout)
	{
		return;
	}

	const ResourceState currentState = currentLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR            ? ResourceState::Present
	                                   : currentLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ? ResourceState::RenderTarget
	                                                                                               : ResourceState::Common;
	const VulkanResourceStateMapping sourceState = currentLayout == VK_IMAGE_LAYOUT_UNDEFINED
	                                                   ? VulkanResourceStateMapping{}
	                                                   : VulkanTypeConversions::ToResourceStateMapping(currentState);
	const VkImageMemoryBarrier2 imageBarrier{
	    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
	    .pNext = nullptr,
	    .srcStageMask = sourceState.StageMask,
	    .srcAccessMask = sourceState.AccessMask,
	    .dstStageMask = destinationState.StageMask,
	    .dstAccessMask = destinationState.AccessMask,
	    .oldLayout = currentLayout,
	    .newLayout = newLayout,
	    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	    .image = m_swapChain->GetCurrentBackBufferImage(),
	    .subresourceRange = VkImageSubresourceRange{
	        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	        .baseMipLevel = 0,
	        .levelCount = 1,
	        .baseArrayLayer = 0,
	        .layerCount = 1}};

	const VkDependencyInfo dependencyInfo{
	    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
	    .pNext = nullptr,
	    .dependencyFlags = 0,
	    .memoryBarrierCount = 0,
	    .pMemoryBarriers = nullptr,
	    .bufferMemoryBarrierCount = 0,
	    .pBufferMemoryBarriers = nullptr,
	    .imageMemoryBarrierCount = 1,
	    .pImageMemoryBarriers = &imageBarrier};
	vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
	currentLayout = newLayout;
}