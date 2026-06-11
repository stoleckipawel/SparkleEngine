#pragma once

#include "../Bindings/RenderBindingSet.h"
#include "../Commands/RenderCommandList.h"
#include "../Core/RhiCapabilities.h"
#include "../Core/RhiBackendApi.h"
#include "../Descriptors/RhiDescriptorHandles.h"
#include "../Diagnostics/RhiDiagnostics.h"
#include "../Formats/PixelFormat.h"
#include "../Interop/RhiNativeHandles.h"
#include "../Memory/RhiMemoryTypes.h"
#include "../Pipeline/RhiPipelineStateDesc.h"
#include "../RayTracing/RhiRayTracingDesc.h"
#include "../Resources/RenderConstantBufferData.h"
#include "../Resources/RhiResourceDesc.h"
#include "../Resources/RhiTextureUpload.h"
#include "../Resources/RhiResourceView.h"
#include "../RHIAPI.h"
#include "../Samplers/RhiSamplerDesc.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>

class Texture;

class SPARKLE_RHI_API RenderHardwareInterface
{
  public:
	virtual ~RenderHardwareInterface() noexcept = default;

	virtual const RhiCapabilities& GetCapabilities() const noexcept = 0;
	virtual ERhiBackendApi GetBackendApi() const noexcept = 0;
	virtual CookedShaderBinaryFormat GetRequiredShaderBinaryFormat() const noexcept = 0;
	virtual std::uint32_t GetCurrentFrameIndex() const noexcept = 0;
	virtual void WaitForIdle() noexcept = 0;
	virtual NativeGraphicsDeviceHandle GetDeviceHandle() const noexcept = 0;
	virtual NativeGraphicsQueueHandle GetGraphicsQueueHandle() const noexcept = 0;
	virtual bool UpgradePresentationInterface(RhiNativeInterfaceUpgradeCallback callback, void* userData) noexcept = 0;
	virtual RenderCommandList& GetGraphicsCommandList(std::uint32_t frameIndex) noexcept = 0;
	virtual RhiRayTracingCapabilities GetRayTracingCapabilities() const noexcept = 0;
	virtual RenderDiagnostics& GetDiagnostics() noexcept = 0;
	virtual const RenderDiagnostics& GetDiagnostics() const noexcept = 0;
	virtual std::unique_ptr<RenderBindingSet> CreateBindingSet(const RenderBindingSetDesc& desc) = 0;
	virtual std::unique_ptr<RenderBindingLayout> CreateBindingLayout(const RenderBindingLayoutCompileDesc& desc) = 0;
	virtual std::unique_ptr<RenderPipelineState> CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc) = 0;
	virtual std::unique_ptr<RenderPipelineState> CreateComputePipelineState(const ComputePipelineStateDesc& desc) = 0;
	virtual void BindGlobalDescriptorState(RenderCommandList& commandList) const noexcept = 0;
	virtual RhiDescriptorAllocation AllocateDescriptor(ERhiDescriptorAllocatorType descriptorType) = 0;
	virtual void ReleaseDescriptor(ERhiDescriptorAllocatorType descriptorType, const RhiDescriptorAllocation& allocation) noexcept = 0;
	virtual RhiDescriptorTableHandle AllocateDescriptorTable(ERhiDescriptorAllocatorType descriptorType, std::uint32_t descriptorCount) = 0;
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
	virtual std::unique_ptr<Texture> CreateTexture(RhiTextureUploadDesc textureUpload, std::wstring_view debugName) = 0;
	virtual RhiOwnedResourceHandle CreateTextureResource(
	    const RhiTextureResourceDesc& desc,
	    ResourceState initialState,
	    RhiMemoryCategory category,
	    RhiMemoryResidencyClass residencyClass,
	    std::wstring_view debugName) = 0;
	virtual RhiOwnedResourceHandle CreateBufferResource(
	    const RhiBufferResourceDesc& desc,
	    ResourceState initialState,
	    RhiMemoryCategory category,
	    RhiMemoryResidencyClass residencyClass,
	    std::wstring_view debugName) = 0;
	virtual bool CreateVertexBuffer(
	    const void* data,
	    std::size_t sizeInBytes,
	    std::uint32_t strideInBytes,
	    std::wstring_view debugName,
	    RhiOwnedResourceHandle& outResource,
	    RhiVertexBufferView& outView) = 0;
	virtual bool CreateStructuredBuffer(
	    const void* data,
	    std::size_t sizeInBytes,
	    std::uint32_t strideInBytes,
	    std::wstring_view debugName,
	    RhiOwnedResourceHandle& outResource,
	    RhiResourceViewHandle& outView) = 0;
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
	virtual RhiOwnedMemoryBlockHandle CreateTransientMemoryBlock(
	    RhiTransientAllocationPool pool,
	    std::uint64_t sizeInBytes,
	    std::uint64_t alignment,
	    std::wstring_view debugName) = 0;
	virtual void ReleaseTransientMemoryBlock(RhiOwnedMemoryBlockHandle memoryBlock) noexcept = 0;
	virtual RhiOwnedResourceHandle CreateAliasingTextureResource(
	    RhiOwnedMemoryBlockHandle memoryBlock,
	    std::uint64_t memoryBlockOffset,
	    const RhiTransientTextureAllocationDesc& desc,
	    std::wstring_view debugName) = 0;
	virtual RhiOwnedResourceHandle CreateAliasingBufferResource(
	    RhiOwnedMemoryBlockHandle memoryBlock,
	    std::uint64_t memoryBlockOffset,
	    const RhiTransientBufferAllocationDesc& desc,
	    std::wstring_view debugName) = 0;
	virtual RhiResourceViewHandle CreateResourceView(const RhiResourceViewDesc& desc) = 0;
	virtual void ReleaseResourceView(RhiResourceViewHandle view) noexcept = 0;
	virtual RhiCpuDescriptorHandle GetResourceViewCpuHandle(RhiResourceViewHandle view) const noexcept = 0;
	virtual RhiGpuDescriptorHandle GetResourceViewGpuHandle(RhiResourceViewHandle view) const noexcept = 0;
	virtual bool SupportsUnorderedAccess(NativeResourceHandle resource) const noexcept = 0;
	virtual void BeginPresentRenderPass(const float clearColor[4]) noexcept = 0;
	virtual void BeginPresentOverlayPass() noexcept = 0;
	virtual void EndPresentRenderPass() noexcept = 0;
	virtual PixelFormat GetPresentColorFormat() const noexcept = 0;
};
