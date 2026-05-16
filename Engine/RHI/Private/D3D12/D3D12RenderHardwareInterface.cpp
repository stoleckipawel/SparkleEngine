#include "PCH.h"

#include "D3D12/D3D12RenderHardwareInterface.h"

#include "D3D12/D3D12RenderCommandList.h"
#include "D3D12/D3D12Rhi.h"
#include "D3D12/D3D12SwapChain.h"
#include "D3D12/D3D12TypeConversions.h"
#include "D3D12/Diagnostics/D3D12RenderDiagnostics.h"
#include "D3D12/Descriptors/D3D12DescriptorHeap.h"
#include "D3D12/Descriptors/D3D12DescriptorHeapManager.h"
#include "D3D12/Memory/D3D12GpuMemoryAllocator.h"
#include "D3D12/Pipeline/D3D12BindingLayout.h"
#include "D3D12/Pipeline/D3D12PipelineState.h"
#include "D3D12/Resources/D3D12ConstantBufferManager.h"
#include "D3D12/Samplers/D3D12SamplerLibrary.h"
#include "D3D12/UI/D3D12ImGuiBackend.h"
#include "Resources/Texture.h"
#include "D3D12/Textures/TextureFactory.h"
#include "D3D12/Textures/TextureLoader.h"
#include "Shaders/CookedShaderPackage.h"

#include <algorithm>
#include <d3d12.h>
#include <wrl/client.h>
#include <cstring>
#include <string>
#include <vector>

D3D12RenderHardwareInterface::D3D12RenderHardwareInterface(
    D3D12Rhi& rhi,
	D3D12GpuMemoryAllocator& memoryAllocator,
    D3D12DescriptorHeapManager& descriptorHeapManager,
    D3D12SwapChain& swapChain,
    D3D12ConstantBufferManager& constantBufferManager) noexcept :
	m_rhi(&rhi), m_memoryAllocator(&memoryAllocator), m_descriptorHeapManager(&descriptorHeapManager), m_swapChain(&swapChain),
	m_constantBufferManager(&constantBufferManager)
{
	for (std::uint32_t frameIndex = 0; frameIndex < RenderConfig::FramesInFlight; ++frameIndex)
	{
		m_commandLists[frameIndex] = std::make_unique<D3D12RenderCommandList>(*this, rhi.GetCommandList(frameIndex).Get());
	}

	m_diagnostics = CreateD3D12RenderDiagnostics(rhi);
	m_imguiBackend = std::make_unique<D3D12ImGuiBackend>(*this);
}

D3D12RenderHardwareInterface::~D3D12RenderHardwareInterface() noexcept = default;

ERhiBackendApi D3D12RenderHardwareInterface::GetBackendApi() const noexcept
{
	return ERhiBackendApi::D3D12;
}

CookedShaderBinaryFormat D3D12RenderHardwareInterface::GetRequiredShaderBinaryFormat() const noexcept
{
	return CookedShaderBinaryFormat::Dxil;
}

std::wstring D3D12RenderHardwareInterface::CopyDebugName(std::wstring_view debugName, std::wstring_view fallbackName)
{
	return debugName.empty() ? std::wstring(fallbackName) : std::wstring(debugName);
}

bool D3D12RenderHardwareInterface::ResourceSupportsUnorderedAccess(ID3D12Resource* resource) noexcept
{
	return resource != nullptr && (resource->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0;
}

RhiOwnedResourceHandle D3D12RenderHardwareInterface::WrapOwnedResource(std::unique_ptr<D3D12GpuAllocationRecord> record) noexcept
{
	return MakeD3D12OwnedResourceHandle(std::move(record));
}

RhiOwnedResourceHandle D3D12RenderHardwareInterface::WrapOwnedResource(
    Microsoft::WRL::ComPtr<ID3D12Resource>&& resource,
    std::wstring debugName) noexcept
{
	if (resource == nullptr)
	{
		return {};
	}

	auto record = std::make_unique<D3D12GpuAllocationRecord>();
	record->Resource = std::move(resource);
	record->DebugName = std::move(debugName);
	return MakeD3D12OwnedResourceHandle(std::move(record));
}

RhiOwnedHeapHandle D3D12RenderHardwareInterface::WrapOwnedHeap(std::unique_ptr<D3D12GpuHeapRecord> record) noexcept
{
	if (record == nullptr)
	{
		return {};
	}

	return MakeD3D12OwnedHeapHandle(std::move(record));
}

std::uint32_t D3D12RenderHardwareInterface::GetCurrentFrameIndex() const noexcept
{
	return m_rhi != nullptr ? m_rhi->GetCurrentFrameIndex() : 0u;
}

void D3D12RenderHardwareInterface::WaitForIdle() noexcept
{
	if (m_rhi != nullptr)
	{
		m_rhi->Flush();
		DrainCompletedOwnedResourceReleases();
	}
}

NativeGraphicsDeviceHandle D3D12RenderHardwareInterface::GetDeviceHandle() const noexcept
{
	return NativeGraphicsDeviceHandle{m_rhi != nullptr ? m_rhi->GetDevice().Get() : nullptr};
}

NativeGraphicsQueueHandle D3D12RenderHardwareInterface::GetGraphicsQueueHandle() const noexcept
{
	return NativeGraphicsQueueHandle{m_rhi != nullptr ? m_rhi->GetCommandQueue().Get() : nullptr};
}

RenderCommandList& D3D12RenderHardwareInterface::GetGraphicsCommandList(std::uint32_t frameIndex) noexcept
{
	DrainCompletedOwnedResourceReleases();
	return *m_commandLists[frameIndex];
}

RhiRayTracingCapabilities D3D12RenderHardwareInterface::GetRayTracingCapabilities() const noexcept
{
	return m_rhi != nullptr ? m_rhi->GetRayTracingCapabilities() : RhiRayTracingCapabilities{};
}

RenderDiagnostics& D3D12RenderHardwareInterface::GetDiagnostics() noexcept
{
	return *m_diagnostics;
}

const RenderDiagnostics& D3D12RenderHardwareInterface::GetDiagnostics() const noexcept
{
	return *m_diagnostics;
}

bool D3D12RenderHardwareInterface::InitializeImGuiBackend()
{
	return m_imguiBackend != nullptr && m_imguiBackend->Initialize();
}

void D3D12RenderHardwareInterface::BeginImGuiFrame() noexcept
{
	if (m_imguiBackend != nullptr)
	{
		m_imguiBackend->BeginFrame();
	}
}

void D3D12RenderHardwareInterface::RenderImGuiDrawData(ImDrawData* drawData) noexcept
{
	if (m_imguiBackend != nullptr)
	{
		RenderCommandList& commandList = GetGraphicsCommandList(GetCurrentFrameIndex());
		m_imguiBackend->Render(commandList.GetNativeHandle(), drawData);
	}
}

void D3D12RenderHardwareInterface::ShutdownImGuiBackend() noexcept
{
	if (m_imguiBackend != nullptr)
	{
		m_imguiBackend->Shutdown();
	}
}

std::unique_ptr<RenderBindingLayout> D3D12RenderHardwareInterface::CreateBindingLayout(const RenderBindingLayoutCompileDesc& desc)
{
	if (m_rhi == nullptr || desc.ParameterLayout == nullptr || desc.ShaderPackage == nullptr)
	{
		return {};
	}

	return D3D12BindingLayoutCompiler::Compile(*m_rhi, desc);
}

std::unique_ptr<RenderPipelineState> D3D12RenderHardwareInterface::CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc)
{
	if (m_rhi == nullptr || desc.BindingLayout == nullptr || !desc.VertexShader.IsValid())
	{
		return {};
	}

	return std::make_unique<D3D12PipelineState>(*m_rhi, desc);
}

std::unique_ptr<RenderPipelineState> D3D12RenderHardwareInterface::CreateComputePipelineState(const ComputePipelineStateDesc& desc)
{
	if (m_rhi == nullptr || desc.BindingLayout == nullptr || !desc.ComputeShader.IsValid())
	{
		return {};
	}

	return std::make_unique<D3D12PipelineState>(*m_rhi, desc);
}

void D3D12RenderHardwareInterface::SetShaderVisibleDescriptorHeaps(RenderCommandList& commandList) const noexcept
{
	if (m_descriptorHeapManager != nullptr)
	{
		m_descriptorHeapManager->SetShaderVisibleHeaps(commandList);
	}
}

NativeDescriptorHeapHandle D3D12RenderHardwareInterface::GetShaderResourceHeapHandle() const noexcept
{
	if (m_descriptorHeapManager == nullptr)
	{
		return {};
	}

	D3D12DescriptorHeap* heap = m_descriptorHeapManager->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	return NativeDescriptorHeapHandle{heap != nullptr ? heap->GetRaw() : nullptr};
}

RhiDescriptorAllocation D3D12RenderHardwareInterface::AllocateDescriptor(ERhiDescriptorHeapType heapType)
{
	RhiDescriptorAllocation allocation{};
	if (m_descriptorHeapManager == nullptr)
	{
		return allocation;
	}

	const D3D12_DESCRIPTOR_HEAP_TYPE nativeType = D3D12TypeConversions::ToDescriptorHeapType(heapType);
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};
	m_descriptorHeapManager->AllocateHandle(nativeType, cpuHandle, gpuHandle);
	allocation.CpuHandle = RhiCpuDescriptorHandle{cpuHandle.ptr};
	allocation.GpuHandle = RhiGpuDescriptorHandle{gpuHandle.ptr};
	return allocation;
}

void D3D12RenderHardwareInterface::ReleaseDescriptor(ERhiDescriptorHeapType heapType, const RhiDescriptorAllocation& allocation) noexcept
{
	if (m_descriptorHeapManager == nullptr || !allocation.CpuHandle)
	{
		return;
	}

	m_descriptorHeapManager->FreeHandle(
	    D3D12TypeConversions::ToDescriptorHeapType(heapType),
	    D3D12_CPU_DESCRIPTOR_HANDLE{allocation.CpuHandle.Value},
	    D3D12_GPU_DESCRIPTOR_HANDLE{allocation.GpuHandle.Value});
}

RhiDescriptorTableHandle D3D12RenderHardwareInterface::AllocateDescriptorTable(
    ERhiDescriptorHeapType heapType,
    std::uint32_t descriptorCount)
{
	if (m_descriptorHeapManager == nullptr || descriptorCount == 0)
	{
		return {};
	}

	const D3D12DescriptorHandle nativeHandle =
	    m_descriptorHeapManager->AllocateContiguous(D3D12TypeConversions::ToDescriptorHeapType(heapType), descriptorCount);
	if (!nativeHandle.IsValid())
	{
		return {};
	}

	DescriptorTableRecord record{};
	record.heapType = heapType;
	record.descriptorCount = descriptorCount;
	record.nativeHandle = nativeHandle;

	if (!m_freeDescriptorTableIndices.empty())
	{
		const std::uint32_t recordIndex = m_freeDescriptorTableIndices.back();
		m_freeDescriptorTableIndices.pop_back();
		m_descriptorTableRecords[recordIndex] = record;
		return RhiDescriptorTableHandle{recordIndex + 1u};
	}

	m_descriptorTableRecords.push_back(record);
	return RhiDescriptorTableHandle{static_cast<std::uint32_t>(m_descriptorTableRecords.size())};
}

RhiCpuDescriptorHandle D3D12RenderHardwareInterface::GetDescriptorTableCpuHandle(
    RhiDescriptorTableHandle tableHandle,
    std::uint32_t descriptorIndex) const noexcept
{
	return RhiCpuDescriptorHandle{ResolveDescriptorTableCpuHandle(tableHandle, descriptorIndex).ptr};
}

void D3D12RenderHardwareInterface::ReleaseDescriptorTable(RhiDescriptorTableHandle tableHandle) noexcept
{
	DescriptorTableRecord* const record = FindDescriptorTableRecord(tableHandle);
	if (record == nullptr || m_descriptorHeapManager == nullptr || !record->IsAllocated())
	{
		return;
	}

	m_descriptorHeapManager->FreeContiguous(
	    D3D12TypeConversions::ToDescriptorHeapType(record->heapType),
	    record->nativeHandle,
	    record->descriptorCount);
	*record = DescriptorTableRecord{};
	m_freeDescriptorTableIndices.push_back(tableHandle.Value - 1u);
}

void D3D12RenderHardwareInterface::AllocateShaderResourceDescriptor(
    RhiCpuDescriptorHandle& outCpuHandle,
    RhiGpuDescriptorHandle& outGpuHandle)
{
	outCpuHandle = {};
	outGpuHandle = {};
	if (m_descriptorHeapManager == nullptr)
	{
		return;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};
	m_descriptorHeapManager->AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, cpuHandle, gpuHandle);
	outCpuHandle.Value = cpuHandle.ptr;
	outGpuHandle.Value = gpuHandle.ptr;
}

void D3D12RenderHardwareInterface::ReleaseShaderResourceDescriptor(
    RhiCpuDescriptorHandle cpuHandle,
    RhiGpuDescriptorHandle gpuHandle) noexcept
{
	if (m_descriptorHeapManager == nullptr || !cpuHandle)
	{
		return;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE nativeCpuHandle{};
	nativeCpuHandle.ptr = cpuHandle.Value;
	D3D12_GPU_DESCRIPTOR_HANDLE nativeGpuHandle{};
	nativeGpuHandle.ptr = gpuHandle.Value;
	m_descriptorHeapManager->FreeHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, nativeCpuHandle, nativeGpuHandle);
}

const PerFrameConstantBufferData& D3D12RenderHardwareInterface::GetPerFrameConstantData() const noexcept
{
	static const PerFrameConstantBufferData emptyData{};
	return m_constantBufferManager != nullptr ? m_constantBufferManager->GetPerFrameData() : emptyData;
}

RhiGpuVirtualAddress D3D12RenderHardwareInterface::GetPerFrameConstantGpuAddress() const noexcept
{
	return m_constantBufferManager != nullptr ? m_constantBufferManager->GetPerFrameGpuAddress() : 0;
}


RhiGpuVirtualAddress D3D12RenderHardwareInterface::AllocateUniformConstantBuffer(const void* data, std::uint32_t sizeInBytes)
{
	return m_constantBufferManager != nullptr ? m_constantBufferManager->AllocateUniform(data, sizeInBytes) : 0;
}

RhiGpuVirtualAddress D3D12RenderHardwareInterface::AllocatePerViewConstantBuffer(const PerViewConstantBufferData& data)
{
	return m_constantBufferManager != nullptr ? m_constantBufferManager->AllocatePerView(data) : 0;
}

RhiGpuVirtualAddress D3D12RenderHardwareInterface::AllocatePerObjectVertexConstants(const PerObjectVSConstantBufferData& data)
{
	return m_constantBufferManager != nullptr ? m_constantBufferManager->UpdatePerObjectVS(data) : 0;
}

RhiGpuVirtualAddress D3D12RenderHardwareInterface::AllocatePerObjectPixelConstants(const PerObjectPSConstantBufferData& data)
{
	return m_constantBufferManager != nullptr ? m_constantBufferManager->UpdatePerObjectPS(data) : 0;
}

RhiDescriptorTableBinding D3D12RenderHardwareInterface::GetSharedSamplerBinding(const RhiSamplerDesc& samplerDesc) const noexcept
{
	D3D12SamplerLibrary::Slot slot = D3D12SamplerLibrary::Slot::Count;
	if (!m_samplerTableHandle || !D3D12SamplerLibrary::TryGetSlot(samplerDesc, slot))
	{
		return {};
	}

	return RhiDescriptorTableBinding{m_samplerTableHandle, static_cast<std::uint32_t>(slot)};
}

RhiViewport D3D12RenderHardwareInterface::GetBackBufferViewport() const noexcept
{
	return m_swapChain != nullptr ? m_swapChain->GetDefaultViewport() : RhiViewport{};
}

RhiRect D3D12RenderHardwareInterface::GetBackBufferScissorRect() const noexcept
{
	return m_swapChain != nullptr ? m_swapChain->GetDefaultScissorRect() : RhiRect{};
}

RhiCpuDescriptorHandle D3D12RenderHardwareInterface::GetBackBufferRenderTargetView() const noexcept
{
	return m_swapChain != nullptr ? RhiCpuDescriptorHandle{m_swapChain->GetCPUHandle().ptr} : RhiCpuDescriptorHandle{};
}

NativeResourceHandle D3D12RenderHardwareInterface::GetBackBufferResource() const noexcept
{
	return NativeResourceHandle{m_swapChain != nullptr ? m_swapChain->GetCurrentResource() : nullptr};
}

std::unique_ptr<Texture> D3D12RenderHardwareInterface::CreateTextureFromPath(const std::filesystem::path& texturePath) const
{
	if (m_rhi == nullptr || m_descriptorHeapManager == nullptr)
	{
		return {};
	}

	TextureLoadResult loadResult = TextureLoader::Load(texturePath);
	if (!loadResult.IsValid())
	{
		return {};
	}

	std::unique_ptr<TextureFactory> textureFactory = TextureFactory::Create(*m_rhi, *m_descriptorHeapManager);
	return textureFactory != nullptr ? textureFactory->CreateTexture(std::move(loadResult)) : std::unique_ptr<Texture>{};
}

bool D3D12RenderHardwareInterface::CreateVertexBuffer(
    const void* data,
    std::size_t sizeInBytes,
    std::uint32_t strideInBytes,
    std::wstring_view debugName,
    RhiOwnedResourceHandle& outResource,
    RhiVertexBufferView& outView)
{
	outResource = {};
	outView = {};
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || data == nullptr || sizeInBytes == 0 || strideInBytes == 0)
	{
		return false;
	}

	D3D12_RESOURCE_DESC resourceDesc = D3D12TypeConversions::BuildBufferResourceDesc(RhiBufferResourceDesc{.SizeInBytes = sizeInBytes});
	std::wstring ownedDebugName = CopyDebugName(debugName, L"VertexBuffer");
	std::unique_ptr<D3D12GpuAllocationRecord> ownedRecord = m_memoryAllocator->CreateBuffer(
	    resourceDesc,
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    RhiMemoryCategory::Mesh,
	    RhiMemoryResidencyClass::HostUpload,
	    ownedDebugName);
	if (ownedRecord == nullptr || ownedRecord->Resource == nullptr)
	{
		return false;
	}

	ID3D12Resource* const ownedResource = ownedRecord->Resource.Get();
	void* mappedData = nullptr;
	const D3D12_RANGE readRange{0, 0};
	if (FAILED(ownedResource->Map(0, &readRange, &mappedData)))
	{
		return false;
	}

	ownedRecord->IsMapped = true;
	ownedRecord->CpuMappedAddress = mappedData;
	std::memcpy(mappedData, data, sizeInBytes);
	ownedResource->Unmap(0, nullptr);
	ownedRecord->IsMapped = false;
	ownedRecord->CpuMappedAddress = nullptr;

	outView = RhiVertexBufferView{
	    .BufferLocation = ownedResource->GetGPUVirtualAddress(),
	    .SizeInBytes = static_cast<std::uint32_t>(sizeInBytes),
	    .StrideInBytes = strideInBytes};
	outResource = WrapOwnedResource(std::move(ownedRecord));
	return true;
}

bool D3D12RenderHardwareInterface::CreateIndexBuffer(
    const void* data,
    std::size_t sizeInBytes,
    RhiIndexFormat format,
    std::wstring_view debugName,
    RhiOwnedResourceHandle& outResource,
    RhiIndexBufferView& outView)
{
	outResource = {};
	outView = {};
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || data == nullptr || sizeInBytes == 0)
	{
		return false;
	}

	D3D12_RESOURCE_DESC resourceDesc = D3D12TypeConversions::BuildBufferResourceDesc(RhiBufferResourceDesc{.SizeInBytes = sizeInBytes});
	std::wstring ownedDebugName = CopyDebugName(debugName, L"IndexBuffer");
	std::unique_ptr<D3D12GpuAllocationRecord> ownedRecord = m_memoryAllocator->CreateBuffer(
	    resourceDesc,
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    RhiMemoryCategory::Mesh,
	    RhiMemoryResidencyClass::HostUpload,
	    ownedDebugName);
	if (ownedRecord == nullptr || ownedRecord->Resource == nullptr)
	{
		return false;
	}

	ID3D12Resource* const ownedResource = ownedRecord->Resource.Get();
	void* mappedData = nullptr;
	const D3D12_RANGE readRange{0, 0};
	if (FAILED(ownedResource->Map(0, &readRange, &mappedData)))
	{
		return false;
	}

	ownedRecord->IsMapped = true;
	ownedRecord->CpuMappedAddress = mappedData;
	std::memcpy(mappedData, data, sizeInBytes);
	ownedResource->Unmap(0, nullptr);
	ownedRecord->IsMapped = false;
	ownedRecord->CpuMappedAddress = nullptr;

	outView = RhiIndexBufferView{
	    .BufferLocation = ownedResource->GetGPUVirtualAddress(),
	    .SizeInBytes = static_cast<std::uint32_t>(sizeInBytes),
	    .Format = format};
	outResource = WrapOwnedResource(std::move(ownedRecord));
	return true;
}

void D3D12RenderHardwareInterface::ReleaseOwnedResource(RhiOwnedResourceHandle resource) noexcept
{
	if (resource.Value == nullptr)
	{
		return;
	}

	std::unique_ptr<D3D12GpuAllocationRecord> ownedRecord = TakeD3D12OwnedResourceHandle(resource);
	if (ownedRecord == nullptr)
	{
		return;
	}

	std::uint64_t retireFenceValue = 0;
	if (m_rhi != nullptr)
	{
		retireFenceValue = m_rhi->GetNextFenceValue();
	}

	DrainCompletedOwnedResourceReleases();
	m_pendingOwnedResourceReleases.push_back(
	    PendingOwnedResourceRelease{.Record = std::move(ownedRecord), .RetireFenceValue = retireFenceValue});
}

void D3D12RenderHardwareInterface::DrainCompletedOwnedResourceReleases() noexcept
{
	if (m_pendingOwnedResourceReleases.empty() && m_pendingOwnedHeapReleases.empty())
	{
		return;
	}

	std::uint64_t completedFenceValue = UINT64_MAX;
	if (m_rhi != nullptr && m_rhi->GetFence())
	{
		completedFenceValue = m_rhi->GetFence()->GetCompletedValue();
	}

	auto eraseBegin = std::remove_if(
	    m_pendingOwnedResourceReleases.begin(),
	    m_pendingOwnedResourceReleases.end(),
	    [completedFenceValue](const PendingOwnedResourceRelease& pendingRelease)
	    {
		    return pendingRelease.Record == nullptr || pendingRelease.RetireFenceValue <= completedFenceValue;
	    });
	m_pendingOwnedResourceReleases.erase(eraseBegin, m_pendingOwnedResourceReleases.end());

	auto heapEraseBegin = std::remove_if(
	    m_pendingOwnedHeapReleases.begin(),
	    m_pendingOwnedHeapReleases.end(),
	    [completedFenceValue](const PendingOwnedHeapRelease& pendingRelease)
	    {
		    return pendingRelease.Record == nullptr || pendingRelease.RetireFenceValue <= completedFenceValue;
	    });
	m_pendingOwnedHeapReleases.erase(heapEraseBegin, m_pendingOwnedHeapReleases.end());
}

NativeResourceHandle D3D12RenderHardwareInterface::GetNativeResource(RhiOwnedResourceHandle resource) const noexcept
{
	return NativeResourceHandle{GetD3D12Resource(resource)};
}

RhiGpuVirtualAddress D3D12RenderHardwareInterface::GetResourceGpuVirtualAddress(RhiOwnedResourceHandle resource) const noexcept
{
	ID3D12Resource* const nativeResource = GetD3D12Resource(resource);
	return nativeResource != nullptr ? nativeResource->GetGPUVirtualAddress() : 0;
}

RhiRayTracingAccelerationStructurePrebuildInfo D3D12RenderHardwareInterface::GetBottomLevelAccelerationStructurePrebuildInfo(
    const RhiRayTracingGeometryDesc& geometry) const noexcept
{
	if (m_rhi == nullptr || geometry.VertexBuffer == 0 || geometry.IndexBuffer == 0)
	{
		return {};
	}

	D3D12_RAYTRACING_GEOMETRY_DESC nativeGeometry{};
	nativeGeometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	nativeGeometry.Flags = geometry.Opaque ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
	nativeGeometry.Triangles.Transform3x4 = 0;
	nativeGeometry.Triangles.IndexFormat = D3D12TypeConversions::ToIndexFormat(geometry.IndexFormat);
	nativeGeometry.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	nativeGeometry.Triangles.IndexCount = geometry.IndexCount;
	nativeGeometry.Triangles.VertexCount = geometry.VertexCount;
	nativeGeometry.Triangles.IndexBuffer = geometry.IndexBuffer;
	nativeGeometry.Triangles.VertexBuffer.StartAddress = geometry.VertexBuffer;
	nativeGeometry.Triangles.VertexBuffer.StrideInBytes = geometry.VertexStrideInBytes;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	inputs.NumDescs = 1;
	inputs.pGeometryDescs = &nativeGeometry;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO nativeInfo{};
	m_rhi->GetDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &nativeInfo);
	return RhiRayTracingAccelerationStructurePrebuildInfo{
	    .ResultDataMaxSizeInBytes = nativeInfo.ResultDataMaxSizeInBytes,
	    .ScratchDataSizeInBytes = nativeInfo.ScratchDataSizeInBytes,
	    .UpdateScratchDataSizeInBytes = nativeInfo.UpdateScratchDataSizeInBytes};
}

RhiRayTracingAccelerationStructurePrebuildInfo D3D12RenderHardwareInterface::GetTopLevelAccelerationStructurePrebuildInfo(
    std::uint32_t instanceCount) const noexcept
{
	if (m_rhi == nullptr || instanceCount == 0)
	{
		return {};
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	inputs.NumDescs = instanceCount;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO nativeInfo{};
	m_rhi->GetDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &nativeInfo);
	return RhiRayTracingAccelerationStructurePrebuildInfo{
	    .ResultDataMaxSizeInBytes = nativeInfo.ResultDataMaxSizeInBytes,
	    .ScratchDataSizeInBytes = nativeInfo.ScratchDataSizeInBytes,
	    .UpdateScratchDataSizeInBytes = nativeInfo.UpdateScratchDataSizeInBytes};
}

RhiOwnedResourceHandle D3D12RenderHardwareInterface::CreateRayTracingScratchBuffer(std::uint64_t sizeInBytes, std::wstring_view debugName)
{
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || sizeInBytes == 0)
	{
		return {};
	}

	const D3D12_RESOURCE_DESC resourceDesc =
	    D3D12TypeConversions::BuildBufferResourceDesc(RhiBufferResourceDesc{.SizeInBytes = sizeInBytes, .AllowUnorderedAccess = true});
	std::wstring ownedDebugName = CopyDebugName(debugName, L"RayTracingScratch");
	std::unique_ptr<D3D12GpuAllocationRecord> ownedRecord = m_memoryAllocator->CreateBuffer(
	    resourceDesc,
	    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
	    RhiMemoryCategory::RayTracing,
	    RhiMemoryResidencyClass::DeviceLocal,
	    ownedDebugName);
	if (ownedRecord == nullptr)
	{
		return {};
	}

	return WrapOwnedResource(std::move(ownedRecord));
}

RhiOwnedResourceHandle D3D12RenderHardwareInterface::CreateRayTracingAccelerationStructureBuffer(
    std::uint64_t sizeInBytes,
    std::wstring_view debugName)
{
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || sizeInBytes == 0)
	{
		return {};
	}

	const D3D12_RESOURCE_DESC resourceDesc =
	    D3D12TypeConversions::BuildBufferResourceDesc(RhiBufferResourceDesc{.SizeInBytes = sizeInBytes, .AllowUnorderedAccess = true});
	std::wstring ownedDebugName = CopyDebugName(debugName, L"RayTracingAccelerationStructure");
	std::unique_ptr<D3D12GpuAllocationRecord> ownedRecord = m_memoryAllocator->CreateBuffer(
	    resourceDesc,
	    D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
	    RhiMemoryCategory::RayTracing,
	    RhiMemoryResidencyClass::DeviceLocal,
	    ownedDebugName);
	if (ownedRecord == nullptr)
	{
		return {};
	}

	return WrapOwnedResource(std::move(ownedRecord));
}

RhiOwnedResourceHandle D3D12RenderHardwareInterface::CreateRayTracingInstanceBuffer(
    const RhiRayTracingInstanceDesc* instances,
    std::uint32_t instanceCount,
    std::wstring_view debugName)
{
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || instances == nullptr || instanceCount == 0)
	{
		return {};
	}

	std::vector<D3D12_RAYTRACING_INSTANCE_DESC> nativeInstances(instanceCount);
	for (std::uint32_t instanceIndex = 0; instanceIndex < instanceCount; ++instanceIndex)
	{
		D3D12_RAYTRACING_INSTANCE_DESC& nativeInstance = nativeInstances[instanceIndex];
		const RhiRayTracingInstanceDesc& source = instances[instanceIndex];
		for (std::uint32_t transformIndex = 0; transformIndex < 12; ++transformIndex)
		{
			nativeInstance.Transform[transformIndex / 4][transformIndex % 4] = source.Transform[transformIndex];
		}
		nativeInstance.InstanceID = source.InstanceID;
		nativeInstance.InstanceMask = source.InstanceMask;
		nativeInstance.InstanceContributionToHitGroupIndex = source.InstanceContributionToHitGroupIndex;
		nativeInstance.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		nativeInstance.AccelerationStructure = source.AccelerationStructure;
	}

	const std::uint64_t sizeInBytes = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * static_cast<std::uint64_t>(nativeInstances.size());
	D3D12_RESOURCE_DESC resourceDesc = D3D12TypeConversions::BuildBufferResourceDesc(RhiBufferResourceDesc{.SizeInBytes = sizeInBytes});
	std::wstring ownedDebugName = CopyDebugName(debugName, L"RayTracingInstanceBuffer");
	std::unique_ptr<D3D12GpuAllocationRecord> ownedRecord = m_memoryAllocator->CreateBuffer(
	    resourceDesc,
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    RhiMemoryCategory::RayTracing,
	    RhiMemoryResidencyClass::HostUpload,
	    ownedDebugName);
	if (ownedRecord == nullptr || ownedRecord->Resource == nullptr)
	{
		return {};
	}

	ID3D12Resource* const ownedResource = ownedRecord->Resource.Get();
	void* mappedData = nullptr;
	const D3D12_RANGE readRange{0, 0};
	if (FAILED(ownedResource->Map(0, &readRange, &mappedData)))
	{
		return {};
	}

	ownedRecord->IsMapped = true;
	ownedRecord->CpuMappedAddress = mappedData;
	std::memcpy(mappedData, nativeInstances.data(), static_cast<std::size_t>(sizeInBytes));
	ownedResource->Unmap(0, nullptr);
	ownedRecord->IsMapped = false;
	ownedRecord->CpuMappedAddress = nullptr;
	return WrapOwnedResource(std::move(ownedRecord));
}

RhiResourceAllocationInfo D3D12RenderHardwareInterface::GetTextureAllocationInfo(const RhiTextureResourceDesc& desc) const noexcept
{
	if (m_rhi == nullptr)
	{
		return {};
	}

	const D3D12_RESOURCE_DESC resourceDesc = D3D12TypeConversions::BuildTextureResourceDesc(desc);
	const D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = m_rhi->GetDevice()->GetResourceAllocationInfo(0, 1, &resourceDesc);
	return RhiResourceAllocationInfo{.SizeInBytes = allocationInfo.SizeInBytes, .Alignment = allocationInfo.Alignment};
}

RhiResourceAllocationInfo D3D12RenderHardwareInterface::GetBufferAllocationInfo(const RhiBufferResourceDesc& desc) const noexcept
{
	if (m_rhi == nullptr)
	{
		return {};
	}

	const D3D12_RESOURCE_DESC resourceDesc = D3D12TypeConversions::BuildBufferResourceDesc(desc);
	const D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = m_rhi->GetDevice()->GetResourceAllocationInfo(0, 1, &resourceDesc);
	return RhiResourceAllocationInfo{.SizeInBytes = allocationInfo.SizeInBytes, .Alignment = allocationInfo.Alignment};
}

RhiOwnedHeapHandle D3D12RenderHardwareInterface::CreateOwnedHeap(
    RhiTransientAllocationPool pool,
    std::uint64_t sizeInBytes,
    std::uint64_t alignment,
    std::wstring_view debugName)
{
	if (m_rhi == nullptr || sizeInBytes == 0)
	{
		return {};
	}

	std::wstring ownedDebugName = CopyDebugName(debugName, L"TransientHeap");
	std::unique_ptr<D3D12GpuHeapRecord> ownedHeap =
	    m_memoryAllocator != nullptr ? m_memoryAllocator->CreateTransientHeap(pool, sizeInBytes, alignment, ownedDebugName) : nullptr;
	if (ownedHeap == nullptr)
	{
		return {};
	}

	return WrapOwnedHeap(std::move(ownedHeap));
}

void D3D12RenderHardwareInterface::ReleaseOwnedHeap(RhiOwnedHeapHandle heap) noexcept
{
	std::unique_ptr<D3D12GpuHeapRecord> ownedHeap = TakeD3D12OwnedHeapHandle(heap);
	if (ownedHeap == nullptr)
	{
		return;
	}

	std::uint64_t retireFenceValue = 0;
	if (m_rhi != nullptr)
	{
		retireFenceValue = m_rhi->GetNextFenceValue();
	}

	DrainCompletedOwnedResourceReleases();
	m_pendingOwnedHeapReleases.push_back(PendingOwnedHeapRelease{.Record = std::move(ownedHeap), .RetireFenceValue = retireFenceValue});
}

RhiOwnedResourceHandle D3D12RenderHardwareInterface::CreatePlacedTextureResource(
    RhiOwnedHeapHandle heap,
    std::uint64_t heapOffset,
    const RhiTransientTextureAllocationDesc& desc,
    std::wstring_view debugName)
{
	D3D12GpuHeapRecord* const ownedHeap = GetD3D12GpuHeapRecord(heap);
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || ownedHeap == nullptr)
	{
		return {};
	}

	const D3D12_RESOURCE_DESC resourceDesc = D3D12TypeConversions::BuildTextureResourceDesc(desc.ResourceDesc);
	const D3D12_CLEAR_VALUE clearValue = D3D12TypeConversions::BuildClearValue(desc.ClearValue);
	const D3D12_CLEAR_VALUE* clearValuePtr = desc.ClearValue.ValueType == RhiOptimizedClearValue::Type::None ? nullptr : &clearValue;
	std::wstring ownedDebugName = CopyDebugName(debugName, L"PlacedTexture");
	std::unique_ptr<D3D12GpuAllocationRecord> ownedResource = m_memoryAllocator->CreateAliasingTexture(
	    *ownedHeap,
	    heapOffset,
	    resourceDesc,
	    D3D12TypeConversions::ToResourceStates(desc.InitialState),
	    clearValuePtr,
	    ownedDebugName);
	if (ownedResource == nullptr)
	{
		return {};
	}

	return WrapOwnedResource(std::move(ownedResource));
}

RhiOwnedResourceHandle D3D12RenderHardwareInterface::CreatePlacedBufferResource(
    RhiOwnedHeapHandle heap,
    std::uint64_t heapOffset,
    const RhiTransientBufferAllocationDesc& desc,
    std::wstring_view debugName)
{
	D3D12GpuHeapRecord* const ownedHeap = GetD3D12GpuHeapRecord(heap);
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || ownedHeap == nullptr)
	{
		return {};
	}

	const D3D12_RESOURCE_DESC resourceDesc = D3D12TypeConversions::BuildBufferResourceDesc(desc.ResourceDesc);
	std::wstring ownedDebugName = CopyDebugName(debugName, L"PlacedBuffer");
	std::unique_ptr<D3D12GpuAllocationRecord> ownedResource = m_memoryAllocator->CreateAliasingBuffer(
	    *ownedHeap,
	    heapOffset,
	    resourceDesc,
	    D3D12TypeConversions::ToResourceStates(desc.InitialState),
	    ownedDebugName);
	if (ownedResource == nullptr)
	{
		return {};
	}

	return WrapOwnedResource(std::move(ownedResource));
}

void D3D12RenderHardwareInterface::CreateRenderTargetView(
    NativeResourceHandle resource,
    PixelFormat format,
    RhiCpuDescriptorHandle destination)
{
	if (m_rhi == nullptr || !resource || !destination)
	{
		return;
	}

	D3D12_RENDER_TARGET_VIEW_DESC viewDesc{};
	viewDesc.Format = D3D12TypeConversions::ToDxgiFormat(format);
	viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	m_rhi->GetDevice()->CreateRenderTargetView(
	    D3D12TypeConversions::ToResource(resource),
	    &viewDesc,
	    D3D12TypeConversions::ToCpuDescriptor(destination));
}

void D3D12RenderHardwareInterface::CreateDepthStencilView(
    NativeResourceHandle resource,
    PixelFormat format,
    RhiCpuDescriptorHandle destination)
{
	if (m_rhi == nullptr || !resource || !destination)
	{
		return;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc{};
	viewDesc.Format = D3D12TypeConversions::ToDxgiFormat(format);
	viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	viewDesc.Flags = D3D12_DSV_FLAG_NONE;
	m_rhi->GetDevice()->CreateDepthStencilView(
	    D3D12TypeConversions::ToResource(resource),
	    &viewDesc,
	    D3D12TypeConversions::ToCpuDescriptor(destination));
}

void D3D12RenderHardwareInterface::CreateTextureShaderResourceView(
    NativeResourceHandle resource,
    PixelFormat format,
    RhiCpuDescriptorHandle destination)
{
	if (m_rhi == nullptr || !resource || !destination)
	{
		return;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc{};
	viewDesc.Format = D3D12TypeConversions::ToDxgiFormat(format);
	viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	viewDesc.Texture2D.MostDetailedMip = 0;
	viewDesc.Texture2D.MipLevels = 1;
	m_rhi->GetDevice()->CreateShaderResourceView(
	    D3D12TypeConversions::ToResource(resource),
	    &viewDesc,
	    D3D12TypeConversions::ToCpuDescriptor(destination));
}

void D3D12RenderHardwareInterface::CreateTextureUnorderedAccessView(
    NativeResourceHandle resource,
    PixelFormat format,
    RhiCpuDescriptorHandle destination)
{
	if (m_rhi == nullptr || !resource || !destination)
	{
		return;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc{};
	viewDesc.Format = D3D12TypeConversions::ToDxgiFormat(format);
	viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	viewDesc.Texture2D.MipSlice = 0;
	viewDesc.Texture2D.PlaneSlice = 0;
	m_rhi->GetDevice()->CreateUnorderedAccessView(
	    D3D12TypeConversions::ToResource(resource),
	    nullptr,
	    &viewDesc,
	    D3D12TypeConversions::ToCpuDescriptor(destination));
}

void D3D12RenderHardwareInterface::CreateBufferShaderResourceView(
    NativeResourceHandle resource,
    std::uint64_t sizeInBytes,
    std::uint32_t strideInBytes,
    RhiCpuDescriptorHandle destination)
{
	if (m_rhi == nullptr || !resource || !destination || sizeInBytes == 0)
	{
		return;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc{};
	viewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	if (strideInBytes > 0)
	{
		viewDesc.Format = DXGI_FORMAT_UNKNOWN;
		viewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		viewDesc.Buffer.StructureByteStride = strideInBytes;
		viewDesc.Buffer.NumElements = static_cast<UINT>(sizeInBytes / strideInBytes);
	}
	else
	{
		viewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		viewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
		viewDesc.Buffer.StructureByteStride = 0;
		viewDesc.Buffer.NumElements = static_cast<UINT>(sizeInBytes / sizeof(std::uint32_t));
	}
	m_rhi->GetDevice()->CreateShaderResourceView(
	    D3D12TypeConversions::ToResource(resource),
	    &viewDesc,
	    D3D12TypeConversions::ToCpuDescriptor(destination));
}

void D3D12RenderHardwareInterface::CreateBufferUnorderedAccessView(
    NativeResourceHandle resource,
    std::uint64_t sizeInBytes,
    std::uint32_t strideInBytes,
    RhiCpuDescriptorHandle destination)
{
	if (m_rhi == nullptr || !resource || !destination || sizeInBytes == 0)
	{
		return;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc{};
	viewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	if (strideInBytes > 0)
	{
		viewDesc.Format = DXGI_FORMAT_UNKNOWN;
		viewDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
		viewDesc.Buffer.StructureByteStride = strideInBytes;
		viewDesc.Buffer.NumElements = static_cast<UINT>(sizeInBytes / strideInBytes);
	}
	else
	{
		viewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		viewDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
		viewDesc.Buffer.StructureByteStride = 0;
		viewDesc.Buffer.NumElements = static_cast<UINT>(sizeInBytes / sizeof(std::uint32_t));
	}
	m_rhi->GetDevice()->CreateUnorderedAccessView(
	    D3D12TypeConversions::ToResource(resource),
	    nullptr,
	    &viewDesc,
	    D3D12TypeConversions::ToCpuDescriptor(destination));
}

void D3D12RenderHardwareInterface::CreateRayTracingAccelerationStructureShaderResourceView(
    RhiGpuVirtualAddress accelerationStructureGpuAddress,
    RhiCpuDescriptorHandle destination)
{
	if (m_rhi == nullptr || accelerationStructureGpuAddress == 0 || !destination)
	{
		return;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc{};
	viewDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	viewDesc.RaytracingAccelerationStructure.Location = accelerationStructureGpuAddress;
	m_rhi->GetDevice()->CreateShaderResourceView(nullptr, &viewDesc, D3D12TypeConversions::ToCpuDescriptor(destination));
}

bool D3D12RenderHardwareInterface::SupportsUnorderedAccess(NativeResourceHandle resource) const noexcept
{
	return ResourceSupportsUnorderedAccess(D3D12TypeConversions::ToResource(resource));
}

void D3D12RenderHardwareInterface::BeginPresentRenderPass(const float clearColor[4]) noexcept
{
	if (m_swapChain == nullptr)
	{
		return;
	}

	NativeResourceHandle presentTexture{m_swapChain->GetCurrentResource()};
	if (!presentTexture)
	{
		return;
	}

	RenderCommandList& commandList = GetGraphicsCommandList(GetCurrentFrameIndex());
	commandList.TransitionResource(presentTexture, ResourceState::Present, ResourceState::RenderTarget);
	SetShaderVisibleDescriptorHeaps(commandList);

	const RhiCpuDescriptorHandle renderTargetView = GetBackBufferRenderTargetView();
	commandList.SetRenderTarget(renderTargetView);

	static constexpr float defaultClearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	commandList.ClearRenderTarget(renderTargetView, clearColor != nullptr ? clearColor : defaultClearColor);
}

void D3D12RenderHardwareInterface::BeginPresentOverlayPass() noexcept
{
	if (m_swapChain == nullptr)
	{
		return;
	}

	NativeResourceHandle presentTexture{m_swapChain->GetCurrentResource()};
	if (!presentTexture)
	{
		return;
	}

	RenderCommandList& commandList = GetGraphicsCommandList(GetCurrentFrameIndex());
	commandList.TransitionResource(presentTexture, ResourceState::Present, ResourceState::RenderTarget);
	SetShaderVisibleDescriptorHeaps(commandList);

	const RhiCpuDescriptorHandle renderTargetView = GetBackBufferRenderTargetView();
	commandList.SetRenderTarget(renderTargetView);
}

void D3D12RenderHardwareInterface::EndPresentRenderPass() noexcept
{
	if (m_swapChain == nullptr)
	{
		return;
	}

	NativeResourceHandle presentTexture{m_swapChain->GetCurrentResource()};
	if (!presentTexture)
	{
		return;
	}

	RenderCommandList& commandList = GetGraphicsCommandList(GetCurrentFrameIndex());
	commandList.TransitionResource(presentTexture, ResourceState::RenderTarget, ResourceState::Present);
}

PixelFormat D3D12RenderHardwareInterface::GetPresentColorFormat() const noexcept
{
	return m_swapChain != nullptr ? m_swapChain->GetBackBufferFormat() : PixelFormat::Unknown;
}

void D3D12RenderHardwareInterface::SetSamplerTableHandle(RhiDescriptorTableHandle samplerTableHandle) noexcept
{
	m_samplerTableHandle = samplerTableHandle;
}

D3D12RenderHardwareInterface::DescriptorTableRecord* D3D12RenderHardwareInterface::FindDescriptorTableRecord(
    RhiDescriptorTableHandle tableHandle) noexcept
{
	if (!tableHandle || tableHandle.Value == 0 || tableHandle.Value > m_descriptorTableRecords.size())
	{
		return nullptr;
	}

	DescriptorTableRecord& record = m_descriptorTableRecords[tableHandle.Value - 1u];
	return record.IsAllocated() ? &record : nullptr;
}

const D3D12RenderHardwareInterface::DescriptorTableRecord* D3D12RenderHardwareInterface::FindDescriptorTableRecord(
    RhiDescriptorTableHandle tableHandle) const noexcept
{
	if (!tableHandle || tableHandle.Value == 0 || tableHandle.Value > m_descriptorTableRecords.size())
	{
		return nullptr;
	}

	const DescriptorTableRecord& record = m_descriptorTableRecords[tableHandle.Value - 1u];
	return record.IsAllocated() ? &record : nullptr;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12RenderHardwareInterface::ResolveDescriptorTableCpuHandle(
    RhiDescriptorTableHandle tableHandle,
    std::uint32_t descriptorIndex) const noexcept
{
	const DescriptorTableRecord* const record = FindDescriptorTableRecord(tableHandle);
	if (record == nullptr || descriptorIndex >= record->descriptorCount)
	{
		return D3D12_CPU_DESCRIPTOR_HANDLE{};
	}

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = record->nativeHandle.GetCPU();
	cpuHandle.ptr += static_cast<SIZE_T>(descriptorIndex) * record->nativeHandle.GetIncrementSize();
	return cpuHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12RenderHardwareInterface::ResolveDescriptorTableGpuHandle(
    RhiDescriptorTableHandle tableHandle,
    std::uint32_t descriptorIndex) const noexcept
{
	const DescriptorTableRecord* const record = FindDescriptorTableRecord(tableHandle);
	if (record == nullptr || descriptorIndex >= record->descriptorCount)
	{
		return D3D12_GPU_DESCRIPTOR_HANDLE{};
	}

	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = record->nativeHandle.GetGPU();
	gpuHandle.ptr += static_cast<UINT64>(descriptorIndex) * record->nativeHandle.GetIncrementSize();
	return gpuHandle;
}
