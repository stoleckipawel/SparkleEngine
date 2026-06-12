#include "Vulkan/VulkanPCH.h"

#include "Vulkan/VulkanRenderHardwareInterface.h"

#include "Config/RenderConfig.h"
#include "Resources/Texture.h"
#include "Shaders/CookedShaderPackage.h"
#include "Vulkan/Commands/VulkanCommandContext.h"
#include "Vulkan/Commands/VulkanRenderCommandList.h"
#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Descriptors/VulkanDescriptorAllocator.h"
#include "Vulkan/Descriptors/VulkanDescriptorManager.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Device/VulkanExternalFeatureInteropCapabilities.h"
#include "Vulkan/Diagnostics/VulkanRenderDiagnostics.h"
#include "Vulkan/Memory/VulkanGpuAllocation.h"
#include "Vulkan/Memory/VulkanGpuMemoryAllocator.h"
#include "Vulkan/Pipeline/VulkanBindingLayout.h"
#include "Vulkan/Pipeline/VulkanPipelineState.h"
#include "Vulkan/Resources/VulkanConstantBufferManager.h"
#include "Vulkan/Samplers/VulkanSamplerLibrary.h"
#include "Vulkan/SwapChain/VulkanSwapChain.h"
#include "Vulkan/Textures/VulkanTextureFactory.h"
#include "Vulkan/Resources/VulkanTexture.h"
#include "Vulkan/UI/VulkanImGuiBackend.h"
#include "Vulkan/VulkanTypeConversions.h"
#include "RHI/Public/Validation/RhiValidation.h"

#include <array>
#include <format>
#include <vector>

static const auto g_vulkanRenderHardwareInterfaceLogger = Logging::GetOrCreateLogger("RHI.Vulkan.Interface");

namespace
{
	VkAccelerationStructureTypeKHR ToVkAccelerationStructureType(ERhiRayTracingAccelerationStructureType type) noexcept
	{
		switch (type)
		{
			case ERhiRayTracingAccelerationStructureType::TopLevel:
				return VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
			case ERhiRayTracingAccelerationStructureType::BottomLevel:
			default:
				return VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		}
	}

	VkAccelerationStructureGeometryKHR BuildBottomLevelGeometry(const RhiRayTracingGeometryDesc& geometry) noexcept
	{
		const VkAccelerationStructureGeometryTrianglesDataKHR triangles{
		    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
		    .pNext = nullptr,
		    .vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
		    .vertexData = VkDeviceOrHostAddressConstKHR{.deviceAddress = geometry.VertexBuffer},
		    .vertexStride = geometry.VertexStrideInBytes,
		    .maxVertex = geometry.VertexCount > 0 ? geometry.VertexCount - 1u : 0u,
		    .indexType = VulkanTypeConversions::ToVkIndexType(geometry.IndexFormat),
		    .indexData = VkDeviceOrHostAddressConstKHR{.deviceAddress = geometry.IndexBuffer},
		    .transformData = VkDeviceOrHostAddressConstKHR{.deviceAddress = 0}};
		return VkAccelerationStructureGeometryKHR{
		    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
		    .pNext = nullptr,
		    .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
		    .geometry = VkAccelerationStructureGeometryDataKHR{.triangles = triangles},
		    .flags = geometry.Opaque ? VK_GEOMETRY_OPAQUE_BIT_KHR : 0u};
	}
}

VulkanRenderHardwareInterface::VulkanRenderHardwareInterface(
    VulkanRhi& rhi,
    VulkanSwapChain& swapChain,
    VulkanCommandContext& commandContext,
    VulkanGpuMemoryAllocator& memoryAllocator) noexcept :
    m_rhi(&rhi), m_swapChain(&swapChain), m_commandContext(&commandContext), m_memoryAllocator(&memoryAllocator)
{
	m_descriptorManager = std::make_unique<VulkanDescriptorManager>(rhi, memoryAllocator);
	m_constantBufferManager = std::make_unique<VulkanConstantBufferManager>(memoryAllocator);
	m_samplerLibrary = std::make_unique<VulkanSamplerLibrary>(rhi, *m_descriptorManager);
	m_textureFactory = std::make_unique<VulkanTextureFactory>(memoryAllocator);
	m_imguiBackend = std::make_unique<VulkanImGuiBackend>(*this);
	for (std::uint32_t frameIndex = 0; frameIndex < RenderConfig::FramesInFlight; ++frameIndex)
	{
		commandContext.GetCommandList(frameIndex).SetRhi(&rhi);
		commandContext.GetCommandList(frameIndex).SetMemoryAllocator(&memoryAllocator);
		commandContext.GetCommandList(frameIndex).SetDescriptorAllocator(&m_descriptorManager->GetAllocator());
	}
	m_diagnostics = CreateVulkanRenderDiagnostics(rhi, memoryAllocator);
	RebuildSwapChainBackBufferViews();
	m_capabilities = BuildCapabilities();
}

VulkanRenderHardwareInterface::~VulkanRenderHardwareInterface() noexcept
{
	m_samplerLibrary.reset();
	m_imguiBackend.reset();
	m_textureFactory.reset();
	m_constantBufferManager.reset();
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

bool VulkanRenderHardwareInterface::UpgradePresentationInterface(RhiNativeInterfaceUpgradeCallback, void*) noexcept
{
	return false;
}

RenderCommandList& VulkanRenderHardwareInterface::GetGraphicsCommandList(std::uint32_t) noexcept
{
	return m_commandContext->GetCommandList(m_currentFrameIndex);
}

RhiRayTracingCapabilities VulkanRenderHardwareInterface::GetRayTracingCapabilities() const noexcept
{
	return m_rhi != nullptr ? m_rhi->GetRayTracingCapabilities() : RhiRayTracingCapabilities{};
}

RhiCapabilities VulkanRenderHardwareInterface::BuildCapabilities() const noexcept
{
	VkPhysicalDeviceProperties properties{};
	if (m_rhi != nullptr && m_rhi->GetPhysicalDevice() != VK_NULL_HANDLE)
	{
		vkGetPhysicalDeviceProperties(m_rhi->GetPhysicalDevice(), &properties);
	}

	RhiCapabilities capabilities{};
	capabilities.BackendApi = ERhiBackendApi::Vulkan;
	capabilities.RequiredShaderBinaryFormat = CookedShaderBinaryFormat::SpirV;
	capabilities.DescriptorModel = ERhiDescriptorModel::DescriptorSets;
	capabilities.BindingLimits = RhiBindingLimits{
	    .MaxDescriptorSets = properties.limits.maxBoundDescriptorSets,
	    .MaxShaderResourceDescriptors = properties.limits.maxDescriptorSetSampledImages + properties.limits.maxDescriptorSetStorageImages +
	                                     properties.limits.maxDescriptorSetUniformBuffers + properties.limits.maxDescriptorSetStorageBuffers,
	    .MaxSamplerDescriptors = properties.limits.maxDescriptorSetSamplers,
	    .MaxDescriptorTableEntries = properties.limits.maxDescriptorSetSampledImages + properties.limits.maxDescriptorSetStorageImages,
	    .MaxPushConstantBytes = properties.limits.maxPushConstantsSize};
	capabilities.UploadReadback = RhiUploadReadbackCapabilities{
	    .SupportsBufferUpload = true,
	    .SupportsTextureUpload = true,
	    .SupportsReadback = true};
	for (std::size_t index = 0; index < capabilities.FormatSupport.size(); ++index)
	{
		capabilities.FormatSupport[index] = QueryFormatSupport(kRhiCapabilityPixelFormats[index]);
	}
	capabilities.SupportsTimestampQueries = false;
	capabilities.RayTracing = m_rhi != nullptr ? m_rhi->GetRayTracingCapabilities() : RhiRayTracingCapabilities{};
	capabilities.SupportsMeshShaders = false;
	capabilities.SupportsTaskShaders = false;
	capabilities.Queues = RhiQueueCapabilities{.SupportsGraphics = true, .SupportsCompute = false, .SupportsCopy = false};
	capabilities.SupportsPresent = m_swapChain != nullptr && m_swapChain->GetBackBufferFormat() != PixelFormat::Unknown;
	capabilities.MemoryAllocator = ERhiMemoryAllocatorBackend::VulkanManaged;
	capabilities.ExternalFeatureInterop = BuildVulkanExternalFeatureInteropCapabilities(m_rhi, m_commandContext != nullptr);
	return capabilities;
}

RhiFormatSupport VulkanRenderHardwareInterface::QueryFormatSupport(PixelFormat format) const noexcept
{
	RhiFormatSupport support{.Format = format};
	if (m_rhi == nullptr || m_rhi->GetPhysicalDevice() == VK_NULL_HANDLE || format == PixelFormat::Unknown)
	{
		return support;
	}

	const VkFormat nativeFormat = VulkanTypeConversions::ToVkFormat(format);
	if (nativeFormat == VK_FORMAT_UNDEFINED)
	{
		return support;
	}

	VkFormatProperties properties{};
	vkGetPhysicalDeviceFormatProperties(m_rhi->GetPhysicalDevice(), nativeFormat, &properties);
	const VkFormatFeatureFlags optimal = properties.optimalTilingFeatures;
	support.SupportsTexture = (optimal & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0 ||
	                          (optimal & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) != 0 ||
	                          (optimal & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0;
	support.SupportsShaderResource = (optimal & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0;
	support.SupportsUnorderedAccess = (optimal & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) != 0;
	support.SupportsRenderTarget = (optimal & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) != 0;
	support.SupportsDepthStencil = (optimal & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0;
	return support;
}

RenderDiagnostics& VulkanRenderHardwareInterface::GetDiagnostics() noexcept
{
	return *m_diagnostics;
}

const RenderDiagnostics& VulkanRenderHardwareInterface::GetDiagnostics() const noexcept
{
	return *m_diagnostics;
}

RhiImGuiRenderer& VulkanRenderHardwareInterface::GetImGuiRenderer() noexcept
{
	return *m_imguiBackend;
}

std::unique_ptr<RenderBindingSet> VulkanRenderHardwareInterface::CreateBindingSet(const RenderBindingSetDesc& desc)
{
	return std::make_unique<RenderBindingSet>(*this, desc);
}

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
	return m_descriptorManager != nullptr ? m_descriptorManager->AllocateDescriptor(descriptorType) : RhiDescriptorAllocation{};
}

void VulkanRenderHardwareInterface::ReleaseDescriptor(
    ERhiDescriptorAllocatorType descriptorType,
    const RhiDescriptorAllocation& allocation) noexcept
{
	if (m_descriptorManager != nullptr)
	{
		m_descriptorManager->ReleaseDescriptor(descriptorType, allocation);
	}
}

RhiDescriptorTableHandle VulkanRenderHardwareInterface::AllocateDescriptorTable(
    ERhiDescriptorAllocatorType descriptorType,
    std::uint32_t descriptorCount)
{
	return m_descriptorManager != nullptr ? m_descriptorManager->AllocateDescriptorTable(descriptorType, descriptorCount)
	                                        : RhiDescriptorTableHandle{};
}

RhiCpuDescriptorHandle VulkanRenderHardwareInterface::GetDescriptorTableCpuHandle(
    RhiDescriptorTableHandle tableHandle,
    std::uint32_t descriptorIndex) const noexcept
{
	return m_descriptorManager != nullptr ? m_descriptorManager->GetDescriptorTableCpuHandle(tableHandle, descriptorIndex)
	                                        : RhiCpuDescriptorHandle{};
}

void VulkanRenderHardwareInterface::ReleaseDescriptorTable(RhiDescriptorTableHandle tableHandle) noexcept
{
	if (m_descriptorManager != nullptr)
	{
		m_descriptorManager->ReleaseDescriptorTable(tableHandle);
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
	static const PerFrameConstantBufferData emptyPerFrameConstants = {};
	return m_constantBufferManager != nullptr ? m_constantBufferManager->GetPerFrameData() : emptyPerFrameConstants;
}

RhiGpuVirtualAddress VulkanRenderHardwareInterface::GetPerFrameConstantGpuAddress() const noexcept
{
	return m_constantBufferManager != nullptr ? m_constantBufferManager->GetPerFrameGpuAddress() : RhiGpuVirtualAddress{};
}

RhiGpuVirtualAddress VulkanRenderHardwareInterface::AllocateUniformConstantBuffer(const void* data, std::uint32_t sizeInBytes)
{
	return m_constantBufferManager != nullptr ? m_constantBufferManager->AllocateUniform(data, sizeInBytes) : RhiGpuVirtualAddress{};
}

RhiGpuVirtualAddress VulkanRenderHardwareInterface::AllocatePerViewConstantBuffer(const PerViewConstantBufferData& data)
{
	return m_constantBufferManager != nullptr ? m_constantBufferManager->AllocatePerView(data) : RhiGpuVirtualAddress{};
}

RhiGpuVirtualAddress VulkanRenderHardwareInterface::AllocatePerObjectVertexConstants(const PerObjectVSConstantBufferData& data)
{
	return m_constantBufferManager != nullptr ? m_constantBufferManager->AllocatePerObjectVertexConstants(data) : RhiGpuVirtualAddress{};
}

RhiGpuVirtualAddress VulkanRenderHardwareInterface::AllocatePerObjectPixelConstants(const PerObjectPSConstantBufferData& data)
{
	return m_constantBufferManager != nullptr ? m_constantBufferManager->AllocatePerObjectPixelConstants(data) : RhiGpuVirtualAddress{};
}

RhiDescriptorTableBinding VulkanRenderHardwareInterface::GetSharedSamplerBinding(const RhiSamplerDesc& samplerDesc) const noexcept
{
	return m_samplerLibrary != nullptr ? m_samplerLibrary->GetSharedSamplerBinding(samplerDesc) : RhiDescriptorTableBinding{};
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

std::unique_ptr<Texture> VulkanRenderHardwareInterface::CreateTexture(RhiTextureUploadDesc textureUpload, std::wstring_view debugName)
{
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || m_descriptorManager == nullptr || !textureUpload.IsValid())
	{
		return {};
	}

	return std::make_unique<VulkanTexture>(
	    *m_rhi,
	    *m_memoryAllocator,
	    *m_descriptorManager,
	    std::move(textureUpload),
	    debugName.empty() ? L"VulkanTexture" : debugName);
}

RhiOwnedResourceHandle VulkanRenderHardwareInterface::CreateTextureResource(
    const RhiTextureResourceDesc& desc,
    ResourceState initialState,
    RhiMemoryCategory category,
    RhiMemoryResidencyClass residencyClass,
    std::wstring_view debugName)
{
	(void) initialState;
	if (m_textureFactory == nullptr || !RhiValidation::ValidateTextureResourceDesc(m_capabilities, desc, "RHI.Vulkan.CreateTextureResource"))
	{
		return {};
	}

	return m_textureFactory->CreateTextureResource(desc, category, residencyClass, debugName);
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
	const VkBufferCreateInfo bufferCreateInfo = VulkanTypeConversions::BuildBufferCreateInfo(
	    desc,
	    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
	        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
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
	    .BufferLocation = record->DeviceAddress != 0 ? record->DeviceAddress : reinterpret_cast<std::uint64_t>(record->Buffer),
	    .SizeInBytes = static_cast<std::uint32_t>(sizeInBytes),
	    .StrideInBytes = strideInBytes};
	outResource = MakeVulkanOwnedResourceHandle(std::move(record));
	return true;
}

bool VulkanRenderHardwareInterface::CreateStructuredBuffer(
    const void* data,
    std::size_t sizeInBytes,
    std::uint32_t strideInBytes,
    std::wstring_view debugName,
    RhiOwnedResourceHandle& outResource,
    RhiResourceViewHandle& outView)
{
	outResource = {};
	outView = {};
	if (m_memoryAllocator == nullptr || data == nullptr || sizeInBytes == 0 || strideInBytes == 0)
	{
		return false;
	}

	const RhiBufferResourceDesc desc{.SizeInBytes = sizeInBytes, .StrideInBytes = strideInBytes};
	const VkBufferCreateInfo bufferCreateInfo = VulkanTypeConversions::BuildBufferCreateInfo(desc);
	std::unique_ptr<VulkanGpuAllocationRecord> record = m_memoryAllocator->CreateBuffer(
	    bufferCreateInfo,
	    RhiMemoryCategory::Mesh,
	    RhiMemoryResidencyClass::HostUpload,
	    debugName.empty() ? L"StructuredBuffer" : debugName);
	if (record == nullptr || record->Buffer == VK_NULL_HANDLE || !m_memoryAllocator->WriteAllocation(*record, data, sizeInBytes))
	{
		return false;
	}

	outResource = MakeVulkanOwnedResourceHandle(std::move(record));
	outView = CreateResourceView(RhiResourceViewDesc::BufferShaderResource(GetNativeResource(outResource), sizeInBytes, strideInBytes));
	if (!outView)
	{
		ReleaseOwnedResource(outResource);
		outResource = {};
		return false;
	}

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
	const VkBufferCreateInfo bufferCreateInfo = VulkanTypeConversions::BuildBufferCreateInfo(
	    desc,
	    VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
	        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
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
	    .BufferLocation = record->DeviceAddress != 0 ? record->DeviceAddress : reinterpret_cast<std::uint64_t>(record->Buffer),
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
	if (record == nullptr)
	{
		return 0;
	}
	return record->DeviceAddress != 0 ? record->DeviceAddress : reinterpret_cast<std::uint64_t>(record->Buffer);
}

RhiRayTracingAccelerationStructurePrebuildInfo VulkanRenderHardwareInterface::GetBottomLevelAccelerationStructurePrebuildInfo(
    const RhiRayTracingGeometryDesc& geometry) const noexcept
{
	if (m_rhi == nullptr || !m_rhi->GetRayTracingCapabilities().SupportsRayTracing ||
	    m_rhi->GetAccelerationStructureBuildSizes() == nullptr ||
	    !RhiValidation::ValidateRayTracingGeometryDesc(geometry, "Vulkan.GetBottomLevelAccelerationStructurePrebuildInfo"))
	{
		return {};
	}

	const VkAccelerationStructureGeometryKHR nativeGeometry = BuildBottomLevelGeometry(geometry);
	const VkAccelerationStructureBuildGeometryInfoKHR buildInfo{
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
	    .pNext = nullptr,
	    .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
	    .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
	    .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
	    .srcAccelerationStructure = VK_NULL_HANDLE,
	    .dstAccelerationStructure = VK_NULL_HANDLE,
	    .geometryCount = 1,
	    .pGeometries = &nativeGeometry,
	    .ppGeometries = nullptr,
	    .scratchData = VkDeviceOrHostAddressKHR{.deviceAddress = 0}};
	const std::uint32_t primitiveCount = geometry.IndexCount / 3u;
	VkAccelerationStructureBuildSizesInfoKHR nativeInfo{
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
	m_rhi->GetAccelerationStructureBuildSizes()(
	    m_rhi->GetDevice(),
	    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
	    &buildInfo,
	    &primitiveCount,
	    &nativeInfo);
	return RhiRayTracingAccelerationStructurePrebuildInfo{
	    .ResultDataMaxSizeInBytes = nativeInfo.accelerationStructureSize,
	    .ScratchDataSizeInBytes = nativeInfo.buildScratchSize,
	    .UpdateScratchDataSizeInBytes = nativeInfo.updateScratchSize};
}

RhiRayTracingAccelerationStructurePrebuildInfo VulkanRenderHardwareInterface::GetTopLevelAccelerationStructurePrebuildInfo(
    std::uint32_t instanceCount) const noexcept
{
	if (m_rhi == nullptr || !m_rhi->GetRayTracingCapabilities().SupportsRayTracing ||
	    m_rhi->GetAccelerationStructureBuildSizes() == nullptr || instanceCount == 0)
	{
		return {};
	}

	const VkAccelerationStructureGeometryInstancesDataKHR instances{
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
	    .pNext = nullptr,
	    .arrayOfPointers = VK_FALSE,
	    .data = VkDeviceOrHostAddressConstKHR{.deviceAddress = 0}};
	const VkAccelerationStructureGeometryKHR geometry{
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
	    .pNext = nullptr,
	    .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
	    .geometry = VkAccelerationStructureGeometryDataKHR{.instances = instances},
	    .flags = VK_GEOMETRY_OPAQUE_BIT_KHR};
	const VkAccelerationStructureBuildGeometryInfoKHR buildInfo{
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
	    .pNext = nullptr,
	    .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
	    .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
	    .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
	    .srcAccelerationStructure = VK_NULL_HANDLE,
	    .dstAccelerationStructure = VK_NULL_HANDLE,
	    .geometryCount = 1,
	    .pGeometries = &geometry,
	    .ppGeometries = nullptr,
	    .scratchData = VkDeviceOrHostAddressKHR{.deviceAddress = 0}};
	VkAccelerationStructureBuildSizesInfoKHR nativeInfo{
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
	m_rhi->GetAccelerationStructureBuildSizes()(
	    m_rhi->GetDevice(),
	    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
	    &buildInfo,
	    &instanceCount,
	    &nativeInfo);
	return RhiRayTracingAccelerationStructurePrebuildInfo{
	    .ResultDataMaxSizeInBytes = nativeInfo.accelerationStructureSize,
	    .ScratchDataSizeInBytes = nativeInfo.buildScratchSize,
	    .UpdateScratchDataSizeInBytes = nativeInfo.updateScratchSize};
}

RhiOwnedResourceHandle VulkanRenderHardwareInterface::CreateRayTracingScratchBuffer(std::uint64_t sizeInBytes, std::wstring_view debugName)
{
	const std::uint64_t scratchAlignment = m_rhi != nullptr ? m_rhi->GetRayTracingCapabilities().ScratchBufferByteAlignment : 0;
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || !m_rhi->GetRayTracingCapabilities().SupportsRayTracing ||
	    !RhiValidation::ValidateRayTracingBufferSize(sizeInBytes, scratchAlignment, "Vulkan.CreateRayTracingScratchBuffer"))
	{
		return {};
	}

	const RhiBufferResourceDesc desc{.SizeInBytes = sizeInBytes, .AllowUnorderedAccess = true};
	const VkBufferCreateInfo bufferCreateInfo =
	    VulkanTypeConversions::BuildBufferCreateInfo(desc, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	std::unique_ptr<VulkanGpuAllocationRecord> record = m_memoryAllocator->CreateBuffer(
	    bufferCreateInfo,
	    RhiMemoryCategory::RayTracing,
	    RhiMemoryResidencyClass::DeviceLocal,
	    debugName.empty() ? L"RayTracingScratch" : debugName);
	return record != nullptr ? MakeVulkanOwnedResourceHandle(std::move(record)) : RhiOwnedResourceHandle{};
}

RhiOwnedResourceHandle VulkanRenderHardwareInterface::CreateRayTracingAccelerationStructureBuffer(
    std::uint64_t sizeInBytes,
    ERhiRayTracingAccelerationStructureType type,
    std::wstring_view debugName)
{
	const std::uint64_t asAlignment =
	    m_rhi != nullptr ? m_rhi->GetRayTracingCapabilities().AccelerationStructureByteAlignment : 0;
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || !m_rhi->GetRayTracingCapabilities().SupportsRayTracing ||
	    m_rhi->GetCreateAccelerationStructure() == nullptr || m_rhi->GetAccelerationStructureDeviceAddress() == nullptr ||
	    !RhiValidation::ValidateRayTracingBufferSize(sizeInBytes, asAlignment, "Vulkan.CreateRayTracingAccelerationStructureBuffer"))
	{
		return {};
	}

	const VkAccelerationStructureTypeKHR nativeType = ToVkAccelerationStructureType(type);
	const RhiBufferResourceDesc desc{.SizeInBytes = sizeInBytes, .AllowUnorderedAccess = true};
	const VkBufferCreateInfo bufferCreateInfo = VulkanTypeConversions::BuildBufferCreateInfo(
	    desc,
	    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
	        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	std::unique_ptr<VulkanGpuAllocationRecord> record = m_memoryAllocator->CreateBuffer(
	    bufferCreateInfo,
	    RhiMemoryCategory::RayTracing,
	    RhiMemoryResidencyClass::DeviceLocal,
	    debugName.empty() ? L"RayTracingAccelerationStructure" : debugName);
	if (record == nullptr || record->Buffer == VK_NULL_HANDLE)
	{
		return {};
	}

	const VkAccelerationStructureCreateInfoKHR createInfo{
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
	    .pNext = nullptr,
	    .createFlags = 0,
	    .buffer = record->Buffer,
	    .offset = 0,
	    .size = sizeInBytes,
	    .type = nativeType,
	    .deviceAddress = 0};
	VkAccelerationStructureKHR accelerationStructure = VK_NULL_HANDLE;
	const VkResult result =
	    m_rhi->GetCreateAccelerationStructure()(m_rhi->GetDevice(), &createInfo, nullptr, &accelerationStructure);
	if (!VulkanResult::Succeeded(result) || accelerationStructure == VK_NULL_HANDLE)
	{
		return {};
	}

	record->AccelerationStructure = accelerationStructure;
	record->AccelerationStructureType = nativeType;
	const VkAccelerationStructureDeviceAddressInfoKHR addressInfo{
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
	    .pNext = nullptr,
	    .accelerationStructure = accelerationStructure};
	record->DeviceAddress = m_rhi->GetAccelerationStructureDeviceAddress()(m_rhi->GetDevice(), &addressInfo);
	SetVulkanAllocationRecordDebugName(*record, debugName.empty() ? L"RayTracingAccelerationStructure" : debugName);
	return record->DeviceAddress != 0 ? MakeVulkanOwnedResourceHandle(std::move(record)) : RhiOwnedResourceHandle{};
}

RhiOwnedResourceHandle VulkanRenderHardwareInterface::CreateRayTracingInstanceBuffer(
    const RhiRayTracingInstanceDesc* instances,
    std::uint32_t instanceCount,
    std::wstring_view debugName)
{
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || !m_rhi->GetRayTracingCapabilities().SupportsRayTracing ||
	    !RhiValidation::ValidateRayTracingInstanceDescs(instances, instanceCount, "Vulkan.CreateRayTracingInstanceBuffer"))
	{
		return {};
	}

	std::vector<VkAccelerationStructureInstanceKHR> nativeInstances(instanceCount);
	for (std::uint32_t instanceIndex = 0; instanceIndex < instanceCount; ++instanceIndex)
	{
		const RhiRayTracingInstanceDesc& source = instances[instanceIndex];
		VkAccelerationStructureInstanceKHR& nativeInstance = nativeInstances[instanceIndex];
		for (std::uint32_t transformIndex = 0; transformIndex < source.Transform.size(); ++transformIndex)
		{
			nativeInstance.transform.matrix[transformIndex / 4][transformIndex % 4] = source.Transform[transformIndex];
		}
		nativeInstance.instanceCustomIndex = source.InstanceID & 0x00FFFFFFu;
		nativeInstance.mask = source.InstanceMask & 0xFFu;
		nativeInstance.instanceShaderBindingTableRecordOffset = source.InstanceContributionToHitGroupIndex & 0x00FFFFFFu;
		nativeInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
		nativeInstance.accelerationStructureReference = source.AccelerationStructure;
	}

	const std::uint64_t sizeInBytes = sizeof(VkAccelerationStructureInstanceKHR) * static_cast<std::uint64_t>(nativeInstances.size());
	const RhiBufferResourceDesc desc{.SizeInBytes = sizeInBytes};
	const VkBufferCreateInfo bufferCreateInfo = VulkanTypeConversions::BuildBufferCreateInfo(
	    desc,
	    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
	std::unique_ptr<VulkanGpuAllocationRecord> record = m_memoryAllocator->CreateBuffer(
	    bufferCreateInfo,
	    RhiMemoryCategory::RayTracing,
	    RhiMemoryResidencyClass::HostUpload,
	    debugName.empty() ? L"RayTracingInstanceBuffer" : debugName);
	if (record == nullptr || record->Buffer == VK_NULL_HANDLE ||
	    !m_memoryAllocator->WriteAllocation(*record, nativeInstances.data(), static_cast<std::size_t>(sizeInBytes)))
	{
		return {};
	}
	return MakeVulkanOwnedResourceHandle(std::move(record));
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
	if (m_memoryAllocator == nullptr || !memoryBlock ||
	    !RhiValidation::ValidateTextureResourceDesc(m_capabilities, desc.ResourceDesc, "RHI.Vulkan.CreateAliasingTextureResource"))
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
	return m_descriptorManager != nullptr ? m_descriptorManager->CreateResourceView(desc) : RhiResourceViewHandle{};
}

void VulkanRenderHardwareInterface::ReleaseResourceView(RhiResourceViewHandle view) noexcept
{
	if (m_descriptorManager != nullptr)
	{
		m_descriptorManager->ReleaseResourceView(view);
	}
}

RhiCpuDescriptorHandle VulkanRenderHardwareInterface::GetResourceViewCpuHandle(RhiResourceViewHandle view) const noexcept
{
	return m_descriptorManager != nullptr ? m_descriptorManager->GetResourceViewCpuHandle(view) : RhiCpuDescriptorHandle{};
}

RhiGpuDescriptorHandle VulkanRenderHardwareInterface::GetResourceViewGpuHandle(RhiResourceViewHandle view) const noexcept
{
	return m_descriptorManager != nullptr ? m_descriptorManager->GetResourceViewGpuHandle(view) : RhiGpuDescriptorHandle{};
}

std::uint64_t VulkanRenderHardwareInterface::ResolveImGuiTextureId(RhiGpuDescriptorHandle shaderResourceView) noexcept
{
	if (m_descriptorManager == nullptr || m_imguiBackend == nullptr)
	{
		return 0;
	}

	const VkImageView imageView = m_descriptorManager->GetRegisteredImageView(shaderResourceView);
	return m_imguiBackend->GetTextureId(imageView);
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

VkInstance VulkanRenderHardwareInterface::GetVulkanInstance() const noexcept
{
	return m_rhi != nullptr ? m_rhi->GetInstance() : VK_NULL_HANDLE;
}

VkPhysicalDevice VulkanRenderHardwareInterface::GetVulkanPhysicalDevice() const noexcept
{
	return m_rhi != nullptr ? m_rhi->GetPhysicalDevice() : VK_NULL_HANDLE;
}

VkDevice VulkanRenderHardwareInterface::GetVulkanDevice() const noexcept
{
	return m_rhi != nullptr ? m_rhi->GetDevice() : VK_NULL_HANDLE;
}

VkQueue VulkanRenderHardwareInterface::GetVulkanGraphicsQueue() const noexcept
{
	return m_rhi != nullptr ? m_rhi->GetGraphicsQueue() : VK_NULL_HANDLE;
}

std::uint32_t VulkanRenderHardwareInterface::GetVulkanGraphicsQueueFamilyIndex() const noexcept
{
	return m_rhi != nullptr ? m_rhi->GetGraphicsQueueFamilyIndex() : UINT32_MAX;
}

std::uint32_t VulkanRenderHardwareInterface::GetVulkanApiVersion() const noexcept
{
	return m_rhi != nullptr ? m_rhi->GetAdapterInfo().ApiVersion : VK_API_VERSION_1_3;
}

std::uint32_t VulkanRenderHardwareInterface::GetSwapChainBackBufferCount() const noexcept
{
	return m_swapChain != nullptr ? m_swapChain->GetBackBufferCount() : 0;
}

VkFormat VulkanRenderHardwareInterface::GetNativeBackBufferFormat() const noexcept
{
	return m_swapChain != nullptr ? m_swapChain->GetNativeBackBufferFormat() : VK_FORMAT_UNDEFINED;
}

void VulkanRenderHardwareInterface::SetCurrentFrameIndex(std::uint32_t frameIndex) noexcept
{
	m_currentFrameIndex = frameIndex;
}

void VulkanRenderHardwareInterface::ResetTransientFrameResources() noexcept
{
	if (m_descriptorManager != nullptr)
	{
		m_descriptorManager->BeginFrame(m_currentFrameIndex);
	}
	if (m_constantBufferManager != nullptr)
	{
		m_constantBufferManager->BeginFrame(m_currentFrameIndex);
	}
}

void VulkanRenderHardwareInterface::RebuildSwapChainBackBufferViews() noexcept
{
	m_swapChainBackBufferLayouts.clear();
	m_isPresentRendering = false;
	if (m_swapChain == nullptr)
	{
		return;
	}

	const std::uint32_t backBufferCount = m_swapChain->GetBackBufferCount();
	m_swapChainBackBufferLayouts.assign(backBufferCount, VK_IMAGE_LAYOUT_UNDEFINED);
	if (m_descriptorManager != nullptr)
	{
		m_descriptorManager->RebuildSwapChainBackBufferViews(*m_swapChain);
	}
}

RhiResourceViewHandle VulkanRenderHardwareInterface::GetCurrentBackBufferViewHandle() const noexcept
{
	if (m_swapChain == nullptr)
	{
		return {};
	}

	const std::uint32_t backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
	return m_descriptorManager != nullptr ? m_descriptorManager->GetSwapChainBackBufferView(backBufferIndex) : RhiResourceViewHandle{};
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
