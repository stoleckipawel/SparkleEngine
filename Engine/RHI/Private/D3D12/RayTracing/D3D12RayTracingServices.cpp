#include "PCH.h"

#include "D3D12/RayTracing/D3D12RayTracingServices.h"

#include "D3D12/D3D12TypeConversions.h"
#include "D3D12/Device/D3D12Rhi.h"
#include "D3D12/Memory/D3D12GpuAllocation.h"
#include "D3D12/Memory/D3D12GpuMemoryAllocator.h"
#include "RHI/Public/Validation/RhiValidation.h"

#include <cstring>
#include <d3d12.h>
#include <memory>
#include <string>
#include <vector>

namespace
{
	std::wstring CopyDebugName(std::wstring_view debugName, std::wstring_view fallbackName)
	{
		return debugName.empty() ? std::wstring(fallbackName) : std::wstring(debugName);
	}
}

D3D12RayTracingServices::D3D12RayTracingServices(D3D12Rhi& rhi, D3D12GpuMemoryAllocator& memoryAllocator) noexcept :
    m_rhi(&rhi), m_memoryAllocator(&memoryAllocator)
{
}

RhiRayTracingCapabilities D3D12RayTracingServices::GetCapabilities() const noexcept
{
	return m_rhi != nullptr ? m_rhi->GetRayTracingCapabilities() : RhiRayTracingCapabilities{};
}

RhiRayTracingAccelerationStructurePrebuildInfo D3D12RayTracingServices::GetBottomLevelAccelerationStructurePrebuildInfo(
    const RhiRayTracingGeometryDesc& geometry) const noexcept
{
	if (m_rhi == nullptr || !m_rhi->GetRayTracingCapabilities().SupportsRayTracing ||
	    !RhiValidation::ValidateRayTracingGeometryDesc(geometry, "D3D12.GetBottomLevelAccelerationStructurePrebuildInfo"))
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
	RhiRayTracingAccelerationStructurePrebuildInfo prebuildInfo{
	    .ResultDataMaxSizeInBytes = nativeInfo.ResultDataMaxSizeInBytes,
	    .ScratchDataSizeInBytes = nativeInfo.ScratchDataSizeInBytes,
	    .UpdateScratchDataSizeInBytes = nativeInfo.UpdateScratchDataSizeInBytes};
	(void)RhiValidation::ValidateRayTracingAccelerationStructurePrebuildInfo(
	    prebuildInfo,
	    "D3D12.GetBottomLevelAccelerationStructurePrebuildInfo");
	return prebuildInfo;
}

RhiRayTracingAccelerationStructurePrebuildInfo D3D12RayTracingServices::GetTopLevelAccelerationStructurePrebuildInfo(
    std::uint32_t instanceCount) const noexcept
{
	if (m_rhi == nullptr || !m_rhi->GetRayTracingCapabilities().SupportsRayTracing)
	{
		return {};
	}
	if (instanceCount == 0)
	{
		(void)RhiValidation::ValidateRayTracingInstanceDescs(nullptr, instanceCount, "D3D12.GetTopLevelAccelerationStructurePrebuildInfo");
		return {};
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	inputs.NumDescs = instanceCount;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO nativeInfo{};
	m_rhi->GetDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &nativeInfo);
	RhiRayTracingAccelerationStructurePrebuildInfo prebuildInfo{
	    .ResultDataMaxSizeInBytes = nativeInfo.ResultDataMaxSizeInBytes,
	    .ScratchDataSizeInBytes = nativeInfo.ScratchDataSizeInBytes,
	    .UpdateScratchDataSizeInBytes = nativeInfo.UpdateScratchDataSizeInBytes};
	(void)RhiValidation::ValidateRayTracingAccelerationStructurePrebuildInfo(
	    prebuildInfo,
	    "D3D12.GetTopLevelAccelerationStructurePrebuildInfo");
	return prebuildInfo;
}

RhiOwnedResourceHandle D3D12RayTracingServices::CreateScratchBuffer(std::uint64_t sizeInBytes, std::wstring_view debugName)
{
	const std::uint64_t scratchAlignment = m_rhi != nullptr ? m_rhi->GetRayTracingCapabilities().ScratchBufferByteAlignment : 0;
	if (m_rhi == nullptr || m_memoryAllocator == nullptr ||
	    !RhiValidation::ValidateRayTracingBufferSize(sizeInBytes, scratchAlignment, "D3D12.CreateRayTracingScratchBuffer"))
	{
		return {};
	}

	const D3D12_RESOURCE_DESC resourceDesc =
	    D3D12TypeConversions::BuildBufferResourceDesc(RhiBufferResourceDesc{.SizeInBytes = sizeInBytes, .AllowUnorderedAccess = true});
	std::unique_ptr<D3D12GpuAllocationRecord> ownedRecord = m_memoryAllocator->CreateBuffer(
	    resourceDesc,
	    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
	    RhiMemoryCategory::RayTracing,
	    RhiMemoryResidencyClass::DeviceLocal,
	    CopyDebugName(debugName, L"RayTracingScratch"));
	return ownedRecord != nullptr ? MakeD3D12OwnedResourceHandle(std::move(ownedRecord)) : RhiOwnedResourceHandle{};
}

RhiOwnedResourceHandle D3D12RayTracingServices::CreateAccelerationStructureBuffer(
    std::uint64_t sizeInBytes,
    ERhiRayTracingAccelerationStructureType,
    std::wstring_view debugName)
{
	const std::uint64_t asAlignment =
	    m_rhi != nullptr ? m_rhi->GetRayTracingCapabilities().AccelerationStructureByteAlignment : 0;
	if (m_rhi == nullptr || m_memoryAllocator == nullptr ||
	    !RhiValidation::ValidateRayTracingBufferSize(sizeInBytes, asAlignment, "D3D12.CreateRayTracingAccelerationStructureBuffer"))
	{
		return {};
	}

	const D3D12_RESOURCE_DESC resourceDesc =
	    D3D12TypeConversions::BuildBufferResourceDesc(RhiBufferResourceDesc{.SizeInBytes = sizeInBytes, .AllowUnorderedAccess = true});
	std::unique_ptr<D3D12GpuAllocationRecord> ownedRecord = m_memoryAllocator->CreateBuffer(
	    resourceDesc,
	    D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
	    RhiMemoryCategory::RayTracing,
	    RhiMemoryResidencyClass::DeviceLocal,
	    CopyDebugName(debugName, L"RayTracingAccelerationStructure"));
	return ownedRecord != nullptr ? MakeD3D12OwnedResourceHandle(std::move(ownedRecord)) : RhiOwnedResourceHandle{};
}

RhiOwnedResourceHandle D3D12RayTracingServices::CreateInstanceBuffer(
    const RhiRayTracingInstanceDesc* instances,
    std::uint32_t instanceCount,
    std::wstring_view debugName)
{
	if (m_rhi == nullptr || m_memoryAllocator == nullptr ||
	    !RhiValidation::ValidateRayTracingInstanceDescs(instances, instanceCount, "D3D12.CreateRayTracingInstanceBuffer"))
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
	const D3D12_RESOURCE_DESC resourceDesc = D3D12TypeConversions::BuildBufferResourceDesc(RhiBufferResourceDesc{.SizeInBytes = sizeInBytes});
	std::unique_ptr<D3D12GpuAllocationRecord> ownedRecord = m_memoryAllocator->CreateBuffer(
	    resourceDesc,
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    RhiMemoryCategory::RayTracing,
	    RhiMemoryResidencyClass::HostUpload,
	    CopyDebugName(debugName, L"RayTracingInstanceBuffer"));
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
	return MakeD3D12OwnedResourceHandle(std::move(ownedRecord));
}
