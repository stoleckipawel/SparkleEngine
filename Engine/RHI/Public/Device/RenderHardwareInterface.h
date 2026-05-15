#pragma once

#include "../Commands/RenderCommandList.h"
#include "../Core/RhiBackendApi.h"
#include "../Descriptors/RhiDescriptorHandles.h"
#include "../Diagnostics/RhiDiagnostics.h"
#include "../Formats/PixelFormat.h"
#include "../Interop/RhiNativeHandles.h"
#include "../Pipeline/RhiPipelineStateDesc.h"
#include "../RayTracing/RhiRayTracingDesc.h"
#include "../Resources/RenderConstantBufferData.h"
#include "../Resources/RhiResourceDesc.h"
#include "../RHIAPI.h"
#include "../Samplers/RhiSamplerDesc.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>

class Texture;
struct ImDrawData;
enum class CookedShaderBinaryFormat : std::uint8_t;

class SPARKLE_RHI_API RenderHardwareInterface
{
  public:
	virtual ~RenderHardwareInterface() noexcept = default;

	virtual ERhiBackendApi GetBackendApi() const noexcept = 0;
	virtual CookedShaderBinaryFormat GetRequiredShaderBinaryFormat() const noexcept = 0;
	virtual std::uint32_t GetCurrentFrameIndex() const noexcept = 0;
	virtual void WaitForIdle() noexcept = 0;
	virtual NativeGraphicsDeviceHandle GetDeviceHandle() const noexcept = 0;
	virtual NativeGraphicsQueueHandle GetGraphicsQueueHandle() const noexcept = 0;
	virtual RenderCommandList& GetGraphicsCommandList(std::uint32_t frameIndex) noexcept = 0;
	virtual NativeGraphicsCommandListHandle GetGraphicsCommandListHandle(std::uint32_t frameIndex) const noexcept = 0;
	virtual RhiRayTracingCapabilities GetRayTracingCapabilities() const noexcept = 0;
	virtual RenderDiagnostics& GetDiagnostics() noexcept = 0;
	virtual const RenderDiagnostics& GetDiagnostics() const noexcept = 0;
	virtual bool InitializeImGuiBackend() = 0;
	virtual void BeginImGuiFrame() noexcept = 0;
	virtual void RenderImGuiDrawData(NativeGraphicsCommandListHandle commandList, ImDrawData* drawData) noexcept = 0;
	virtual void ShutdownImGuiBackend() noexcept = 0;
	virtual std::unique_ptr<RenderBindingLayout> CreateBindingLayout(const RenderBindingLayoutCompileDesc& desc) = 0;
	virtual std::unique_ptr<RenderPipelineState> CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc) = 0;
	virtual std::unique_ptr<RenderPipelineState> CreateComputePipelineState(const ComputePipelineStateDesc& desc) = 0;
	virtual void SetShaderVisibleDescriptorHeaps(RenderCommandList& commandList) const noexcept = 0;
	virtual NativeDescriptorHeapHandle GetShaderResourceHeapHandle() const noexcept = 0;
	virtual RhiDescriptorAllocation AllocateDescriptor(ERhiDescriptorHeapType heapType) = 0;
	virtual void ReleaseDescriptor(ERhiDescriptorHeapType heapType, const RhiDescriptorAllocation& allocation) noexcept = 0;
	virtual RhiDescriptorTableHandle AllocateDescriptorTable(ERhiDescriptorHeapType heapType, std::uint32_t descriptorCount) = 0;
	virtual RhiCpuDescriptorHandle GetDescriptorTableCpuHandle(RhiDescriptorTableHandle tableHandle, std::uint32_t descriptorIndex = 0)
	    const noexcept = 0;
	virtual void ReleaseDescriptorTable(RhiDescriptorTableHandle tableHandle) noexcept = 0;
	virtual void AllocateShaderResourceDescriptor(RhiCpuDescriptorHandle& outCpuHandle, RhiGpuDescriptorHandle& outGpuHandle) = 0;
	virtual void ReleaseShaderResourceDescriptor(RhiCpuDescriptorHandle cpuHandle, RhiGpuDescriptorHandle gpuHandle) noexcept = 0;
	virtual const PerFrameConstantBufferData& GetPerFrameConstantData() const noexcept = 0;
	virtual RhiGpuVirtualAddress GetPerFrameConstantGpuAddress() const noexcept = 0;
	virtual RhiGpuVirtualAddress AllocateUniformConstantBuffer(const void* data, std::uint32_t sizeInBytes) = 0;
	virtual RhiGpuVirtualAddress AllocatePerViewConstantBuffer(const PerViewConstantBufferData& data) = 0;
	virtual RhiGpuVirtualAddress AllocatePerObjectVertexConstants(const PerObjectVSConstantBufferData& data) = 0;
	virtual RhiGpuVirtualAddress AllocatePerObjectPixelConstants(const PerObjectPSConstantBufferData& data) = 0;
	virtual RhiDescriptorTableBinding GetSharedSamplerBinding(const RhiSamplerDesc& samplerDesc) const noexcept = 0;
	virtual RhiViewport GetBackBufferViewport() const noexcept = 0;
	virtual RhiRect GetBackBufferScissorRect() const noexcept = 0;
	virtual RhiCpuDescriptorHandle GetBackBufferRenderTargetView() const noexcept = 0;
	virtual NativeResourceHandle GetBackBufferResource() const noexcept = 0;
	virtual std::unique_ptr<Texture> CreateTextureFromPath(const std::filesystem::path& texturePath) const = 0;
	virtual bool CreateVertexBuffer(
	    const void* data,
	    std::size_t sizeInBytes,
	    std::uint32_t strideInBytes,
	    std::wstring_view debugName,
	    RhiOwnedResourceHandle& outResource,
	    RhiVertexBufferView& outView) = 0;
	virtual bool CreateIndexBuffer(
	    const void* data,
	    std::size_t sizeInBytes,
	    RhiIndexFormat format,
	    std::wstring_view debugName,
	    RhiOwnedResourceHandle& outResource,
	    RhiIndexBufferView& outView) = 0;
	virtual void ReleaseOwnedResource(RhiOwnedResourceHandle resource) noexcept = 0;
	virtual NativeResourceHandle GetNativeResource(RhiOwnedResourceHandle resource) const noexcept = 0;
	virtual RhiGpuVirtualAddress GetResourceGpuVirtualAddress(RhiOwnedResourceHandle resource) const noexcept = 0;
	virtual RhiRayTracingAccelerationStructurePrebuildInfo GetBottomLevelAccelerationStructurePrebuildInfo(
	    const RhiRayTracingGeometryDesc& geometry) const noexcept = 0;
	virtual RhiRayTracingAccelerationStructurePrebuildInfo GetTopLevelAccelerationStructurePrebuildInfo(
	    std::uint32_t instanceCount) const noexcept = 0;
	virtual RhiOwnedResourceHandle CreateRayTracingScratchBuffer(std::uint64_t sizeInBytes, std::wstring_view debugName) = 0;
	virtual RhiOwnedResourceHandle CreateRayTracingAccelerationStructureBuffer(std::uint64_t sizeInBytes, std::wstring_view debugName) = 0;
	virtual RhiOwnedResourceHandle CreateRayTracingInstanceBuffer(
	    const RhiRayTracingInstanceDesc* instances,
	    std::uint32_t instanceCount,
	    std::wstring_view debugName) = 0;
	virtual RhiResourceAllocationInfo GetTextureAllocationInfo(const RhiTextureResourceDesc& desc) const noexcept = 0;
	virtual RhiResourceAllocationInfo GetBufferAllocationInfo(const RhiBufferResourceDesc& desc) const noexcept = 0;
	virtual RhiOwnedHeapHandle CreateOwnedHeap(
	    RhiTransientAllocationPool pool,
	    std::uint64_t sizeInBytes,
	    std::uint64_t alignment,
	    std::wstring_view debugName) = 0;
	virtual void ReleaseOwnedHeap(RhiOwnedHeapHandle heap) noexcept = 0;
	virtual RhiOwnedResourceHandle CreatePlacedTextureResource(
	    RhiOwnedHeapHandle heap,
	    std::uint64_t heapOffset,
	    const RhiTransientTextureAllocationDesc& desc,
	    std::wstring_view debugName) = 0;
	virtual RhiOwnedResourceHandle CreatePlacedBufferResource(
	    RhiOwnedHeapHandle heap,
	    std::uint64_t heapOffset,
	    const RhiTransientBufferAllocationDesc& desc,
	    std::wstring_view debugName) = 0;
	virtual void CreateRenderTargetView(NativeResourceHandle resource, PixelFormat format, RhiCpuDescriptorHandle destination) = 0;
	virtual void CreateDepthStencilView(NativeResourceHandle resource, PixelFormat format, RhiCpuDescriptorHandle destination) = 0;
	virtual void CreateTextureShaderResourceView(NativeResourceHandle resource, PixelFormat format, RhiCpuDescriptorHandle destination) = 0;
	virtual void CreateTextureUnorderedAccessView(
	    NativeResourceHandle resource,
	    PixelFormat format,
	    RhiCpuDescriptorHandle destination) = 0;
	virtual void CreateBufferShaderResourceView(
	    NativeResourceHandle resource,
	    std::uint64_t sizeInBytes,
	    std::uint32_t strideInBytes,
	    RhiCpuDescriptorHandle destination) = 0;
	virtual void CreateBufferUnorderedAccessView(
	    NativeResourceHandle resource,
	    std::uint64_t sizeInBytes,
	    std::uint32_t strideInBytes,
	    RhiCpuDescriptorHandle destination) = 0;
	virtual void CreateRayTracingAccelerationStructureShaderResourceView(
	    RhiGpuVirtualAddress accelerationStructureGpuAddress,
	    RhiCpuDescriptorHandle destination) = 0;
	virtual bool SupportsUnorderedAccess(NativeResourceHandle resource) const noexcept = 0;
	virtual void TransitionResource(
	    NativeGraphicsCommandListHandle commandList,
	    NativeResourceHandle resource,
	    ResourceState before,
	    ResourceState after) const noexcept = 0;
	virtual void BeginPresentRenderPass(NativeGraphicsCommandListHandle commandList, const float clearColor[4]) const noexcept = 0;
	virtual void BeginPresentOverlayPass(NativeGraphicsCommandListHandle commandList) const noexcept = 0;
	virtual void EndPresentRenderPass(NativeGraphicsCommandListHandle commandList) const noexcept = 0;
	virtual PixelFormat GetPresentColorFormat() const noexcept = 0;
};
