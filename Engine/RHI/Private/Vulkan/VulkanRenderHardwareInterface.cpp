#include "Vulkan/VulkanPCH.h"

#include "Vulkan/VulkanRenderHardwareInterface.h"

#include "Shaders/CookedShaderPackage.h"
#include "Vulkan/Commands/VulkanRenderCommandList.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Diagnostics/VulkanRenderDiagnostics.h"

#include <format>

static const auto g_vulkanRenderHardwareInterfaceLogger = Logging::GetOrCreateLogger("RHI.Vulkan.Interface");

VulkanRenderHardwareInterface::VulkanRenderHardwareInterface(VulkanRhi& rhi) noexcept : m_rhi(&rhi)
{
	m_diagnostics = CreateVulkanRenderDiagnostics(rhi);
	m_placeholderCommandList = std::make_unique<VulkanRenderCommandList>();
}

VulkanRenderHardwareInterface::~VulkanRenderHardwareInterface() noexcept = default;

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
	return 0;
}

void VulkanRenderHardwareInterface::WaitForIdle() noexcept
{
	if (m_rhi != nullptr)
	{
		m_rhi->WaitForIdle();
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
	return *m_placeholderCommandList;
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

std::unique_ptr<RenderBindingLayout> VulkanRenderHardwareInterface::CreateBindingLayout(const RenderBindingLayoutCompileDesc&)
{
	FailRenderingNotImplemented("CreateBindingLayout");
	return {};
}

std::unique_ptr<RenderPipelineState> VulkanRenderHardwareInterface::CreateGraphicsPipelineState(const GraphicsPipelineStateDesc&)
{
	FailRenderingNotImplemented("CreateGraphicsPipelineState");
	return {};
}

std::unique_ptr<RenderPipelineState> VulkanRenderHardwareInterface::CreateComputePipelineState(const ComputePipelineStateDesc&)
{
	FailRenderingNotImplemented("CreateComputePipelineState");
	return {};
}

void VulkanRenderHardwareInterface::BindGlobalDescriptorState(RenderCommandList&) const noexcept {}

RhiDescriptorAllocation VulkanRenderHardwareInterface::AllocateDescriptor(ERhiDescriptorAllocatorType)
{
	FailRenderingNotImplemented("AllocateDescriptor");
	return {};
}

void VulkanRenderHardwareInterface::ReleaseDescriptor(ERhiDescriptorAllocatorType, const RhiDescriptorAllocation&) noexcept {}

RhiDescriptorTableHandle VulkanRenderHardwareInterface::AllocateDescriptorTable(ERhiDescriptorAllocatorType, std::uint32_t)
{
	FailRenderingNotImplemented("AllocateDescriptorTable");
	return {};
}

RhiCpuDescriptorHandle VulkanRenderHardwareInterface::GetDescriptorTableCpuHandle(RhiDescriptorTableHandle, std::uint32_t) const noexcept
{
	return {};
}

void VulkanRenderHardwareInterface::ReleaseDescriptorTable(RhiDescriptorTableHandle) noexcept {}

void VulkanRenderHardwareInterface::AllocateShaderResourceDescriptor(RhiCpuDescriptorHandle& outCpuHandle, RhiGpuDescriptorHandle& outGpuHandle)
{
	FailRenderingNotImplemented("AllocateShaderResourceDescriptor");
	outCpuHandle = {};
	outGpuHandle = {};
}

void VulkanRenderHardwareInterface::ReleaseShaderResourceDescriptor(RhiCpuDescriptorHandle, RhiGpuDescriptorHandle) noexcept {}

const PerFrameConstantBufferData& VulkanRenderHardwareInterface::GetPerFrameConstantData() const noexcept
{
	return m_emptyPerFrameConstants;
}

RhiGpuVirtualAddress VulkanRenderHardwareInterface::GetPerFrameConstantGpuAddress() const noexcept
{
	return {};
}

RhiGpuVirtualAddress VulkanRenderHardwareInterface::AllocateUniformConstantBuffer(const void*, std::uint32_t)
{
	FailRenderingNotImplemented("AllocateUniformConstantBuffer");
	return {};
}

RhiGpuVirtualAddress VulkanRenderHardwareInterface::AllocatePerViewConstantBuffer(const PerViewConstantBufferData&)
{
	FailRenderingNotImplemented("AllocatePerViewConstantBuffer");
	return {};
}

RhiGpuVirtualAddress VulkanRenderHardwareInterface::AllocatePerObjectVertexConstants(const PerObjectVSConstantBufferData&)
{
	FailRenderingNotImplemented("AllocatePerObjectVertexConstants");
	return {};
}

RhiGpuVirtualAddress VulkanRenderHardwareInterface::AllocatePerObjectPixelConstants(const PerObjectPSConstantBufferData&)
{
	FailRenderingNotImplemented("AllocatePerObjectPixelConstants");
	return {};
}

RhiDescriptorTableBinding VulkanRenderHardwareInterface::GetSharedSamplerBinding(const RhiSamplerDesc&) const noexcept
{
	return {};
}

RhiViewport VulkanRenderHardwareInterface::GetBackBufferViewport() const noexcept
{
	return {};
}

RhiRect VulkanRenderHardwareInterface::GetBackBufferScissorRect() const noexcept
{
	return {};
}

RhiCpuDescriptorHandle VulkanRenderHardwareInterface::GetBackBufferRenderTargetView() const noexcept
{
	return {};
}

NativeResourceHandle VulkanRenderHardwareInterface::GetBackBufferResource() const noexcept
{
	return {};
}

std::unique_ptr<Texture> VulkanRenderHardwareInterface::CreateTextureFromPath(const std::filesystem::path&) const
{
	FailRenderingNotImplemented("CreateTextureFromPath");
	return {};
}

bool VulkanRenderHardwareInterface::CreateVertexBuffer(
	const void*,
	std::size_t,
	std::uint32_t,
	std::wstring_view,
	RhiOwnedResourceHandle& outResource,
	RhiVertexBufferView& outView)
{
	FailRenderingNotImplemented("CreateVertexBuffer");
	outResource = {};
	outView = {};
	return false;
}

bool VulkanRenderHardwareInterface::CreateIndexBuffer(
	const void*,
	std::size_t,
	RhiIndexFormat,
	std::wstring_view,
	RhiOwnedResourceHandle& outResource,
	RhiIndexBufferView& outView)
{
	FailRenderingNotImplemented("CreateIndexBuffer");
	outResource = {};
	outView = {};
	return false;
}

void VulkanRenderHardwareInterface::ReleaseOwnedResource(RhiOwnedResourceHandle) noexcept {}

NativeResourceHandle VulkanRenderHardwareInterface::GetNativeResource(RhiOwnedResourceHandle) const noexcept
{
	return {};
}

RhiGpuVirtualAddress VulkanRenderHardwareInterface::GetResourceGpuVirtualAddress(RhiOwnedResourceHandle) const noexcept
{
	return {};
}

RhiRayTracingAccelerationStructurePrebuildInfo VulkanRenderHardwareInterface::GetBottomLevelAccelerationStructurePrebuildInfo(
	const RhiRayTracingGeometryDesc&) const noexcept
{
	return {};
}

RhiRayTracingAccelerationStructurePrebuildInfo VulkanRenderHardwareInterface::GetTopLevelAccelerationStructurePrebuildInfo(std::uint32_t) const noexcept
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

RhiOwnedResourceHandle VulkanRenderHardwareInterface::CreateRayTracingInstanceBuffer(const RhiRayTracingInstanceDesc*, std::uint32_t, std::wstring_view)
{
	FailRenderingNotImplemented("CreateRayTracingInstanceBuffer");
	return {};
}

RhiResourceAllocationInfo VulkanRenderHardwareInterface::GetTextureAllocationInfo(const RhiTextureResourceDesc&) const noexcept
{
	return {};
}

RhiResourceAllocationInfo VulkanRenderHardwareInterface::GetBufferAllocationInfo(const RhiBufferResourceDesc&) const noexcept
{
	return {};
}

RhiOwnedMemoryBlockHandle VulkanRenderHardwareInterface::CreateTransientMemoryBlock(
	RhiTransientAllocationPool,
	std::uint64_t,
	std::uint64_t,
	std::wstring_view)
{
	FailRenderingNotImplemented("CreateTransientMemoryBlock");
	return {};
}

void VulkanRenderHardwareInterface::ReleaseTransientMemoryBlock(RhiOwnedMemoryBlockHandle) noexcept {}

RhiOwnedResourceHandle VulkanRenderHardwareInterface::CreateAliasingTextureResource(
	RhiOwnedMemoryBlockHandle,
	std::uint64_t,
	const RhiTransientTextureAllocationDesc&,
	std::wstring_view)
{
	FailRenderingNotImplemented("CreateAliasingTextureResource");
	return {};
}

RhiOwnedResourceHandle VulkanRenderHardwareInterface::CreateAliasingBufferResource(
	RhiOwnedMemoryBlockHandle,
	std::uint64_t,
	const RhiTransientBufferAllocationDesc&,
	std::wstring_view)
{
	FailRenderingNotImplemented("CreateAliasingBufferResource");
	return {};
}

RhiResourceViewHandle VulkanRenderHardwareInterface::CreateResourceView(const RhiResourceViewDesc&)
{
	FailRenderingNotImplemented("CreateResourceView");
	return {};
}

void VulkanRenderHardwareInterface::ReleaseResourceView(RhiResourceViewHandle) noexcept {}

RhiCpuDescriptorHandle VulkanRenderHardwareInterface::GetResourceViewCpuHandle(RhiResourceViewHandle) const noexcept
{
	return {};
}

RhiGpuDescriptorHandle VulkanRenderHardwareInterface::GetResourceViewGpuHandle(RhiResourceViewHandle) const noexcept
{
	return {};
}

bool VulkanRenderHardwareInterface::SupportsUnorderedAccess(NativeResourceHandle) const noexcept
{
	return false;
}

void VulkanRenderHardwareInterface::BeginPresentRenderPass(const float[4]) noexcept {}

void VulkanRenderHardwareInterface::BeginPresentOverlayPass() noexcept {}

void VulkanRenderHardwareInterface::EndPresentRenderPass() noexcept {}

PixelFormat VulkanRenderHardwareInterface::GetPresentColorFormat() const noexcept
{
	return PixelFormat::Unknown;
}

void VulkanRenderHardwareInterface::FailRenderingNotImplemented(std::string_view operation) noexcept
{
	Diagnostics::Fail(
	    g_vulkanRenderHardwareInterfaceLogger,
	    __FILE__,
	    __LINE__,
	    std::format("Vulkan RHI operation '{}' is not implemented before the resource, command, swapchain, and pipeline phases.", operation));
}