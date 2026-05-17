#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Commands/VulkanRenderCommandList.h"

ERhiBackendApi VulkanRenderCommandList::GetBackendApi() const noexcept
{
	return ERhiBackendApi::Vulkan;
}

NativeGraphicsCommandListHandle VulkanRenderCommandList::GetNativeHandle() const noexcept
{
	return {};
}

bool VulkanRenderCommandList::SupportsDiagnosticScopes() const noexcept
{
	return false;
}

void VulkanRenderCommandList::BeginDiagnosticScope(std::string_view, RhiDiagnosticLabelColor) noexcept {}

void VulkanRenderCommandList::EndDiagnosticScope() noexcept {}

void VulkanRenderCommandList::InsertDiagnosticMarker(std::string_view, RhiDiagnosticLabelColor) noexcept {}

void VulkanRenderCommandList::SetPipelineState(const RenderPipelineState&) noexcept {}

void VulkanRenderCommandList::SetGraphicsBindingLayout(const RenderBindingLayout&) noexcept {}

void VulkanRenderCommandList::SetComputeBindingLayout(const RenderBindingLayout&) noexcept {}

void VulkanRenderCommandList::BindGraphicsConstantBuffer(std::uint32_t, RhiGpuVirtualAddress) noexcept {}

void VulkanRenderCommandList::BindGraphicsShaderResource(std::uint32_t, RhiGpuVirtualAddress) noexcept {}

void VulkanRenderCommandList::BindGraphicsUnorderedAccess(std::uint32_t, RhiGpuVirtualAddress) noexcept {}

void VulkanRenderCommandList::BindGraphicsDescriptorTable(std::uint32_t, RhiDescriptorTableBinding) noexcept {}

void VulkanRenderCommandList::BindGraphicsDescriptorTable(std::uint32_t, RhiGpuDescriptorHandle) noexcept {}

void VulkanRenderCommandList::SetGraphicsPushConstants(std::uint32_t, std::uint32_t, const void*, std::uint32_t) noexcept {}

void VulkanRenderCommandList::BindComputeConstantBuffer(std::uint32_t, RhiGpuVirtualAddress) noexcept {}

void VulkanRenderCommandList::BindComputeShaderResource(std::uint32_t, RhiGpuVirtualAddress) noexcept {}

void VulkanRenderCommandList::BindComputeUnorderedAccess(std::uint32_t, RhiGpuVirtualAddress) noexcept {}

void VulkanRenderCommandList::BindComputeDescriptorTable(std::uint32_t, RhiDescriptorTableBinding) noexcept {}

void VulkanRenderCommandList::BindComputeDescriptorTable(std::uint32_t, RhiGpuDescriptorHandle) noexcept {}

void VulkanRenderCommandList::SetComputePushConstants(std::uint32_t, std::uint32_t, const void*, std::uint32_t) noexcept {}

void VulkanRenderCommandList::SetPrimitiveTopology(RhiPrimitiveTopology) noexcept {}

void VulkanRenderCommandList::BindVertexBuffer(const RhiVertexBufferView&) noexcept {}

void VulkanRenderCommandList::BindIndexBuffer(const RhiIndexBufferView&) noexcept {}

void VulkanRenderCommandList::SetRenderTarget(RhiCpuDescriptorHandle, const RhiCpuDescriptorHandle*) noexcept {}

void VulkanRenderCommandList::SetRenderTargets(std::uint32_t, const RhiCpuDescriptorHandle*, const RhiCpuDescriptorHandle*) noexcept {}

void VulkanRenderCommandList::ClearRenderTarget(RhiCpuDescriptorHandle, const float[4]) noexcept {}

void VulkanRenderCommandList::ClearDepthStencil(RhiCpuDescriptorHandle, float, std::uint8_t) noexcept {}

void VulkanRenderCommandList::SetViewport(const RhiViewport&) noexcept {}

void VulkanRenderCommandList::SetScissorRect(const RhiRect&) noexcept {}

void VulkanRenderCommandList::DrawIndexedInstanced(std::uint32_t, std::uint32_t, std::uint32_t, std::int32_t, std::uint32_t) noexcept {}

void VulkanRenderCommandList::DrawInstanced(std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t) noexcept {}

void VulkanRenderCommandList::Dispatch(std::uint32_t, std::uint32_t, std::uint32_t) noexcept {}

void VulkanRenderCommandList::BuildBottomLevelAccelerationStructure(
	const RhiRayTracingGeometryDesc&,
	RhiGpuVirtualAddress,
	RhiGpuVirtualAddress) noexcept
{
}

void VulkanRenderCommandList::BuildTopLevelAccelerationStructure(
	RhiGpuVirtualAddress,
	std::uint32_t,
	RhiGpuVirtualAddress,
	RhiGpuVirtualAddress) noexcept
{
}

void VulkanRenderCommandList::CopyResource(NativeResourceHandle, NativeResourceHandle) noexcept {}

void VulkanRenderCommandList::AliasResource(NativeResourceHandle, NativeResourceHandle) noexcept {}

void VulkanRenderCommandList::TransitionResource(NativeResourceHandle, ResourceState, ResourceState) noexcept {}

void VulkanRenderCommandList::UnorderedAccessBarrier(NativeResourceHandle) noexcept {}