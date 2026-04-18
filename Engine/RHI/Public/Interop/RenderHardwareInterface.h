#pragma once

#include "ResourceState.h"
#include "../Formats/CompareOp.h"
#include "../Formats/PixelFormat.h"
#include "../Resources/RenderConstantBufferData.h"
#include "../RHIAPI.h"
#include "../Shaders/ShaderStage.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>

class PassParameterLayout;
class LoadedShaderPackage;
class Texture;

enum class RhiBackendApi : std::uint8_t
{
	Unknown = 0,
	D3D12 = 1,
	Vulkan = 2,
};

struct NativeGraphicsDeviceHandle
{
	void* Value = nullptr;

	constexpr explicit operator bool() const noexcept { return Value != nullptr; }
};

struct NativeGraphicsQueueHandle
{
	void* Value = nullptr;

	constexpr explicit operator bool() const noexcept { return Value != nullptr; }
};

struct NativeGraphicsCommandListHandle
{
	void* Value = nullptr;

	constexpr explicit operator bool() const noexcept { return Value != nullptr; }
};

struct NativeDescriptorHeapHandle
{
	void* Value = nullptr;

	constexpr explicit operator bool() const noexcept { return Value != nullptr; }
};

struct NativeResourceHandle
{
	void* Value = nullptr;

	constexpr explicit operator bool() const noexcept { return Value != nullptr; }
};

using NativeTextureHandle = NativeResourceHandle;

struct RhiOwnedHeapHandle
{
	void* Value = nullptr;

	constexpr explicit operator bool() const noexcept { return Value != nullptr; }
};

struct RhiOwnedResourceHandle
{
	void* Value = nullptr;

	constexpr explicit operator bool() const noexcept { return Value != nullptr; }
};

struct RhiCpuDescriptorHandle
{
	std::uintptr_t Value = 0;

	constexpr explicit operator bool() const noexcept { return Value != 0; }
};

struct RhiGpuDescriptorHandle
{
	std::uint64_t Value = 0;

	constexpr explicit operator bool() const noexcept { return Value != 0; }
};

enum class RhiDescriptorHeapType : std::uint8_t
{
	ShaderResource = 0,
	Sampler = 1,
	RenderTarget = 2,
	DepthStencil = 3,
};

struct RhiDescriptorAllocation
{
	RhiCpuDescriptorHandle CpuHandle = {};
	RhiGpuDescriptorHandle GpuHandle = {};

	constexpr bool IsValid() const noexcept { return static_cast<bool>(CpuHandle); }
};

struct RhiDescriptorTableHandle
{
	std::uint32_t Value = 0;

	constexpr explicit operator bool() const noexcept { return Value != 0; }
};

using RhiGpuVirtualAddress = std::uint64_t;

enum class RhiCullMode : std::uint8_t
{
	None = 0,
	Front = 1,
	Back = 2,
};

enum class RhiStencilOp : std::uint8_t
{
	Keep = 0,
	Zero = 1,
	Replace = 2,
	IncrementClamp = 3,
	DecrementClamp = 4,
	Invert = 5,
	IncrementWrap = 6,
	DecrementWrap = 7,
};

enum class RhiVertexLayoutKind : std::uint8_t
{
	StaticMesh = 0,
};

enum class RhiPrimitiveTopology : std::uint8_t
{
	TriangleList = 0,
};

enum class RhiIndexFormat : std::uint8_t
{
	UInt16 = 0,
	UInt32 = 1,
};

struct RhiVertexBufferView
{
	RhiGpuVirtualAddress BufferLocation = 0;
	std::uint32_t SizeInBytes = 0;
	std::uint32_t StrideInBytes = 0;
};

struct RhiIndexBufferView
{
	RhiGpuVirtualAddress BufferLocation = 0;
	std::uint32_t SizeInBytes = 0;
	RhiIndexFormat Format = RhiIndexFormat::UInt32;
};

struct RhiViewport
{
	float X = 0.0f;
	float Y = 0.0f;
	float Width = 0.0f;
	float Height = 0.0f;
	float MinDepth = 0.0f;
	float MaxDepth = 1.0f;
};

struct RhiRect
{
	std::int32_t Left = 0;
	std::int32_t Top = 0;
	std::int32_t Right = 0;
	std::int32_t Bottom = 0;
};

enum class RhiTransientAllocationPool : std::uint8_t
{
	Color = 0,
	Depth = 1,
	Buffer = 2,
};

struct RhiTextureResourceDesc
{
	std::uint32_t Width = 1;
	std::uint32_t Height = 1;
	PixelFormat Format = PixelFormat::Unknown;
	std::uint16_t MipLevels = 1;
	bool AllowRenderTarget = false;
	bool AllowDepthStencil = false;
	bool AllowUnorderedAccess = false;
};

struct RhiBufferResourceDesc
{
	std::uint64_t SizeInBytes = 0;
	std::uint32_t StrideInBytes = 0;
	bool AllowUnorderedAccess = false;
};

struct RhiResourceAllocationInfo
{
	std::uint64_t SizeInBytes = 0;
	std::uint64_t Alignment = 0;
};

struct RhiOptimizedClearValue
{
	enum class Type : std::uint8_t
	{
		None = 0,
		Color = 1,
		DepthStencil = 2,
	};

	Type ValueType = Type::None;
	PixelFormat Format = PixelFormat::Unknown;
	std::array<float, 4> Color = {0.0f, 0.0f, 0.0f, 1.0f};
	float Depth = 1.0f;
	std::uint8_t Stencil = 0;
};

struct RhiTransientTextureAllocationDesc
{
	RhiTextureResourceDesc ResourceDesc = {};
	RhiOptimizedClearValue ClearValue = {};
	ResourceState InitialState = ResourceState::Common;
};

struct RhiTransientBufferAllocationDesc
{
	RhiBufferResourceDesc ResourceDesc = {};
	ResourceState InitialState = ResourceState::Common;
};

enum class CompiledBindingType : std::uint8_t
{
	RootConstantBufferView,
	RootShaderResourceView,
	RootUnorderedAccessView,
	DescriptorTableShaderResourceView,
	DescriptorTableUnorderedAccessView,
	DescriptorTableSampler,
	RootConstants,
};

struct CompiledBinding
{
	const char* Name = nullptr;
	CompiledBindingType Type = CompiledBindingType::DescriptorTableShaderResourceView;
	std::uint32_t RootParameterIndex = 0;
	std::uint32_t ShaderRegister = 0;
	std::uint32_t RegisterSpace = 0;
	std::uint32_t DescriptorCount = 0;
};

struct RenderBindingLayoutCompileDesc
{
	const PassParameterLayout* ParameterLayout = nullptr;
	const LoadedShaderPackage* ShaderPackage = nullptr;
	bool AllowInputAssemblerInputLayout = false;
	const wchar_t* DebugName = L"RHI_BindingLayout";
	bool InlineUniformDataAsRootConstants = false;
};

class SPARKLE_RHI_API RenderBindingLayout
{
  public:
	virtual ~RenderBindingLayout() noexcept = default;

	virtual const PassParameterLayout& GetParameterLayout() const noexcept = 0;
	virtual const CompiledBinding* GetBindings() const noexcept = 0;
	virtual std::size_t GetBindingCount() const noexcept = 0;
	virtual const CompiledBinding* FindBinding(const char* name) const noexcept = 0;
};

struct RhiDepthTestDesc
{
	bool DepthEnable = true;
	bool DepthWriteEnable = true;
	CompareOp DepthFunc = CompareOp::Less;
};

struct RhiStencilTestDesc
{
	bool StencilEnable = false;
	std::uint8_t StencilReadMask = 0xFF;
	std::uint8_t StencilWriteMask = 0xFF;
	CompareOp FrontFaceStencilFunc = CompareOp::Always;
	RhiStencilOp FrontFaceStencilFailOp = RhiStencilOp::Keep;
	RhiStencilOp FrontFaceStencilDepthFailOp = RhiStencilOp::Keep;
	RhiStencilOp FrontFaceStencilPassOp = RhiStencilOp::Keep;
	CompareOp BackFaceStencilFunc = CompareOp::Always;
	RhiStencilOp BackFaceStencilFailOp = RhiStencilOp::Keep;
	RhiStencilOp BackFaceStencilDepthFailOp = RhiStencilOp::Keep;
	RhiStencilOp BackFaceStencilPassOp = RhiStencilOp::Keep;
};

struct RhiShaderStageDesc
{
	const LoadedShaderPackage* Package = nullptr;
	ShaderStage Stage = ShaderStage::Count;

	constexpr bool IsValid() const noexcept { return Package != nullptr && Stage != ShaderStage::Count; }
	explicit constexpr operator bool() const noexcept { return IsValid(); }
};

struct GraphicsPipelineStateDesc
{
	RhiVertexLayoutKind VertexLayout = RhiVertexLayoutKind::StaticMesh;
	const RenderBindingLayout* BindingLayout = nullptr;
	RhiShaderStageDesc VertexShader = {};
	RhiShaderStageDesc PixelShader = {};
	bool RenderWireframe = false;
	RhiCullMode CullMode = RhiCullMode::Back;
	RhiDepthTestDesc DepthTest = {};
	RhiStencilTestDesc StencilTest = {};
	std::array<PixelFormat, 8> RenderTargetFormats = {};
	std::uint32_t RenderTargetCount = 1;
	PixelFormat DepthStencilFormat = PixelFormat::Unknown;
	const wchar_t* DebugName = L"RHI_GraphicsPipelineState";
};

struct ComputePipelineStateDesc
{
	const RenderBindingLayout* BindingLayout = nullptr;
	RhiShaderStageDesc ComputeShader = {};
	const wchar_t* DebugName = L"RHI_ComputePipelineState";
};

class SPARKLE_RHI_API RenderPipelineState
{
  public:
	virtual ~RenderPipelineState() noexcept = default;
};

class SPARKLE_RHI_API RenderCommandList
{
  public:
	virtual ~RenderCommandList() noexcept = default;

	virtual RhiBackendApi GetBackendApi() const noexcept = 0;
	virtual NativeGraphicsCommandListHandle GetNativeHandle() const noexcept = 0;
	virtual void SetDescriptorHeaps(std::uint32_t heapCount, const NativeDescriptorHeapHandle* heaps) noexcept = 0;
	virtual void SetPipelineState(const RenderPipelineState& pipelineState) noexcept = 0;
	virtual void SetGraphicsBindingLayout(const RenderBindingLayout& bindingLayout) noexcept = 0;
	virtual void SetComputeBindingLayout(const RenderBindingLayout& bindingLayout) noexcept = 0;
	virtual void BindGraphicsConstantBuffer(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept = 0;
	virtual void BindGraphicsShaderResource(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept = 0;
	virtual void BindGraphicsUnorderedAccess(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept = 0;
	virtual void BindGraphicsDescriptorTable(std::uint32_t rootParameterIndex, RhiDescriptorTableHandle tableHandle) noexcept = 0;
	virtual void BindGraphicsDescriptorTable(std::uint32_t rootParameterIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept = 0;
	virtual void SetGraphicsRootConstants(
	    std::uint32_t rootParameterIndex,
	    std::uint32_t num32BitValues,
	    const void* data,
	    std::uint32_t destOffsetIn32BitValues) noexcept = 0;
	virtual void BindComputeConstantBuffer(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept = 0;
	virtual void BindComputeShaderResource(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept = 0;
	virtual void BindComputeUnorderedAccess(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept = 0;
	virtual void BindComputeDescriptorTable(std::uint32_t rootParameterIndex, RhiDescriptorTableHandle tableHandle) noexcept = 0;
	virtual void BindComputeDescriptorTable(std::uint32_t rootParameterIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept = 0;
	virtual void SetComputeRootConstants(
	    std::uint32_t rootParameterIndex,
	    std::uint32_t num32BitValues,
	    const void* data,
	    std::uint32_t destOffsetIn32BitValues) noexcept = 0;
	virtual void SetPrimitiveTopology(RhiPrimitiveTopology topology) noexcept = 0;
	virtual void BindVertexBuffer(const RhiVertexBufferView& view) noexcept = 0;
	virtual void BindIndexBuffer(const RhiIndexBufferView& view) noexcept = 0;
	virtual void SetRenderTarget(RhiCpuDescriptorHandle rtv, const RhiCpuDescriptorHandle* dsv = nullptr) noexcept = 0;
	virtual void SetRenderTargets(
	    std::uint32_t numRTVs,
	    const RhiCpuDescriptorHandle* rtvs,
	    const RhiCpuDescriptorHandle* dsv = nullptr) noexcept = 0;
	virtual void ClearRenderTarget(RhiCpuDescriptorHandle rtv, const float color[4]) noexcept = 0;
	virtual void ClearDepthStencil(RhiCpuDescriptorHandle dsv, float depth, std::uint8_t stencil = 0) noexcept = 0;
	virtual void SetViewport(const RhiViewport& viewport) noexcept = 0;
	virtual void SetScissorRect(const RhiRect& rect) noexcept = 0;
	virtual void DrawIndexedInstanced(
	    std::uint32_t indexCountPerInstance,
	    std::uint32_t instanceCount,
	    std::uint32_t startIndexLocation,
	    std::int32_t baseVertexLocation,
	    std::uint32_t startInstanceLocation) noexcept = 0;
	virtual void DrawInstanced(
	    std::uint32_t vertexCountPerInstance,
	    std::uint32_t instanceCount,
	    std::uint32_t startVertexLocation,
	    std::uint32_t startInstanceLocation) noexcept = 0;
	virtual void Dispatch(std::uint32_t groupCountX, std::uint32_t groupCountY, std::uint32_t groupCountZ) noexcept = 0;
	virtual void CopyResource(NativeResourceHandle destinationResource, NativeResourceHandle sourceResource) noexcept = 0;
	virtual void AliasResource(NativeResourceHandle beforeResource, NativeResourceHandle afterResource) noexcept = 0;
	virtual void TransitionResource(NativeResourceHandle resource, ResourceState before, ResourceState after) noexcept = 0;
	virtual void UnorderedAccessBarrier(NativeResourceHandle resource) noexcept = 0;
};

class SPARKLE_RHI_API RenderHardwareInterface
{
  public:
	virtual ~RenderHardwareInterface() noexcept = default;

	virtual RhiBackendApi GetBackendApi() const noexcept = 0;
	virtual std::uint32_t GetCurrentFrameIndex() const noexcept = 0;
	virtual NativeGraphicsDeviceHandle GetDeviceHandle() const noexcept = 0;
	virtual NativeGraphicsQueueHandle GetGraphicsQueueHandle() const noexcept = 0;
	virtual RenderCommandList& GetGraphicsCommandList(std::uint32_t frameIndex) noexcept = 0;
	virtual NativeGraphicsCommandListHandle GetGraphicsCommandListHandle(std::uint32_t frameIndex) const noexcept = 0;
	virtual std::unique_ptr<RenderBindingLayout> CreateBindingLayout(const RenderBindingLayoutCompileDesc& desc) = 0;
	virtual std::unique_ptr<RenderPipelineState> CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc) = 0;
	virtual std::unique_ptr<RenderPipelineState> CreateComputePipelineState(const ComputePipelineStateDesc& desc) = 0;
	virtual void SetShaderVisibleDescriptorHeaps(RenderCommandList& commandList) const noexcept = 0;
	virtual NativeDescriptorHeapHandle GetShaderResourceHeapHandle() const noexcept = 0;
	virtual RhiDescriptorAllocation AllocateDescriptor(RhiDescriptorHeapType heapType) = 0;
	virtual void ReleaseDescriptor(RhiDescriptorHeapType heapType, const RhiDescriptorAllocation& allocation) noexcept = 0;
	virtual RhiDescriptorTableHandle AllocateDescriptorTable(RhiDescriptorHeapType heapType, std::uint32_t descriptorCount) = 0;
	virtual RhiCpuDescriptorHandle GetDescriptorTableCpuHandle(RhiDescriptorTableHandle tableHandle, std::uint32_t descriptorIndex = 0)
	    const noexcept = 0;
	virtual void ReleaseDescriptorTable(RhiDescriptorTableHandle tableHandle) noexcept = 0;
	virtual void AllocateShaderResourceDescriptor(RhiCpuDescriptorHandle& outCpuHandle, RhiGpuDescriptorHandle& outGpuHandle) = 0;
	virtual void ReleaseShaderResourceDescriptor(RhiCpuDescriptorHandle cpuHandle, RhiGpuDescriptorHandle gpuHandle) noexcept = 0;
	virtual const PerFrameConstantBufferData& GetPerFrameConstantData() const noexcept = 0;
	virtual RhiGpuVirtualAddress GetPerFrameConstantGpuAddress() const noexcept = 0;
	virtual RhiGpuVirtualAddress AllocatePerViewConstantBuffer(const PerViewConstantBufferData& data) = 0;
	virtual RhiGpuVirtualAddress AllocatePerObjectVertexConstants(const PerObjectVSConstantBufferData& data) = 0;
	virtual RhiGpuVirtualAddress AllocatePerObjectPixelConstants(const PerObjectPSConstantBufferData& data) = 0;
	virtual RhiDescriptorTableHandle GetSamplerTableHandle() const noexcept = 0;
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
	virtual bool SupportsUnorderedAccess(NativeResourceHandle resource) const noexcept = 0;
	virtual void TransitionResource(
	    NativeGraphicsCommandListHandle commandList,
	    NativeResourceHandle resource,
	    ResourceState before,
	    ResourceState after) const noexcept = 0;
	virtual void BeginPresentRenderPass(NativeGraphicsCommandListHandle commandList, const float clearColor[4]) const noexcept = 0;
	virtual void EndPresentRenderPass(NativeGraphicsCommandListHandle commandList) const noexcept = 0;
	virtual PixelFormat GetPresentColorFormat() const noexcept = 0;
};