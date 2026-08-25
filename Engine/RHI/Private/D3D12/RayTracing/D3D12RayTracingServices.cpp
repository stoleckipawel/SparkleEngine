#include "PCH.h"

#include "D3D12/RayTracing/D3D12RayTracingServices.h"
#include "D3D12/RayTracing/D3D12RayTracingShaderTable.h"

#include "D3D12/D3D12TypeConversions.h"
#include "D3D12/Device/D3D12Rhi.h"
#include "D3D12/Memory/D3D12GpuAllocation.h"
#include "D3D12/Memory/D3D12GpuMemoryAllocator.h"
#include "Validation/RhiContract.h"

#include <cstring>
#include <d3d12.h>
#include <memory>
#include <string>

namespace D3D12RayTracingText
{
	std::wstring MakeDebugName(std::wstring_view debugName, std::wstring_view defaultDebugName)
	{
		return debugName.empty() ? std::wstring(defaultDebugName) : std::wstring(debugName);
	}
}

D3D12RayTracingServices::D3D12RayTracingServices(
    D3D12Rhi& rhi,
    D3D12GpuMemoryAllocator& memoryAllocator,
    D3D12NvapiRayTracingProvider& nvapiProvider) noexcept :
    m_rhi(&rhi),
    m_memoryAllocator(&memoryAllocator),
    m_nvapiProvider(&nvapiProvider),
    m_classicTlasServices(rhi, memoryAllocator),
    m_partitionedTlasServices(rhi, memoryAllocator, nvapiProvider)
{
}

RhiClassicTlasService& D3D12RayTracingServices::GetClassicTlasService() noexcept
{
	return m_classicTlasServices;
}

const RhiClassicTlasService& D3D12RayTracingServices::GetClassicTlasService() const noexcept
{
	return m_classicTlasServices;
}

RhiPartitionedTlasService& D3D12RayTracingServices::GetPartitionedTlasService() noexcept
{
	return m_partitionedTlasServices;
}

const RhiPartitionedTlasService& D3D12RayTracingServices::GetPartitionedTlasService() const noexcept
{
	return m_partitionedTlasServices;
}

RhiRayTracingCapabilities D3D12RayTracingServices::GetCapabilities() const noexcept
{
	return m_rhi != nullptr ? m_rhi->GetRayTracingCapabilities() : RhiRayTracingCapabilities{};
}

RhiRayTracingAccelerationStructurePrebuildInfo D3D12RayTracingServices::GetBottomLevelAccelerationStructurePrebuildInfo(
    const RhiRayTracingGeometryDesc& geometry) const noexcept
{
	if (m_rhi == nullptr || !m_rhi->GetRayTracingCapabilities().SupportsAccelerationStructure
	    || !RhiContract::IsRayTracingGeometryDescUsable(geometry))
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
	nativeGeometry.Triangles.IndexBuffer = 0;
	nativeGeometry.Triangles.VertexBuffer.StartAddress = 0;
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
	return prebuildInfo;
}

RhiRayTracingAccelerationStructurePrebuildInfo D3D12RayTracingServices::GetTopLevelAccelerationStructurePrebuildInfo(
    std::uint32_t instanceCount,
    ERhiClassicTlasBuildFlags buildFlags) const noexcept
{
	return m_classicTlasServices.GetClassicTopLevelAccelerationStructurePrebuildInfo(instanceCount, buildFlags);
}

RhiPartitionedTlasBuildSizes D3D12RayTracingServices::GetPartitionedTopLevelAccelerationStructureBuildSizes(
    const RhiPartitionedTlasDesc& desc) const noexcept
{
	return m_partitionedTlasServices.GetPartitionedTopLevelAccelerationStructureBuildSizes(desc);
}

RhiOwnedResourceHandle D3D12RayTracingServices::CreatePartitionedTopLevelAccelerationStructureBuffer(
    const RhiPartitionedTlasBuildSizes& sizes,
    std::wstring_view debugName)
{
	return m_partitionedTlasServices.CreatePartitionedTopLevelAccelerationStructureBuffer(sizes, debugName);
}

RhiOwnedResourceHandle D3D12RayTracingServices::CreatePartitionedTopLevelAccelerationStructureOperationBuffer(
    const RhiPartitionedTlasOperationPackDesc& operationPack,
    std::wstring_view debugName)
{
	return m_partitionedTlasServices.CreatePartitionedTopLevelAccelerationStructureOperationBuffer(operationPack, debugName);
}

RhiOwnedResourceHandle D3D12RayTracingServices::CreateScratchBuffer(std::uint64_t sizeInBytes, std::wstring_view debugName)
{
	const std::uint64_t scratchAlignment = m_rhi != nullptr ? m_rhi->GetRayTracingCapabilities().ScratchBufferByteAlignment : 0;
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || !RhiContract::IsRayTracingBufferSizeUsable(sizeInBytes, scratchAlignment))
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
	    D3D12RayTracingText::MakeDebugName(debugName, L"RayTracingScratch"));
	return ownedRecord != nullptr ? MakeD3D12OwnedResourceHandle(std::move(ownedRecord)) : RhiOwnedResourceHandle{};
}

RhiOwnedResourceHandle D3D12RayTracingServices::CreateRayTracingScratchBuffer(std::uint64_t sizeInBytes, std::wstring_view debugName)
{
	return CreateScratchBuffer(sizeInBytes, debugName);
}

RhiOwnedResourceHandle D3D12RayTracingServices::CreateAccelerationStructureBuffer(
    std::uint64_t sizeInBytes,
    ERhiRayTracingAccelerationStructureType,
    std::wstring_view debugName)
{
	const std::uint64_t asAlignment = m_rhi != nullptr ? m_rhi->GetRayTracingCapabilities().AccelerationStructureByteAlignment : 0;
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || !RhiContract::IsRayTracingBufferSizeUsable(sizeInBytes, asAlignment))
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
	    D3D12RayTracingText::MakeDebugName(debugName, L"RayTracingAccelerationStructure"));
	return ownedRecord != nullptr ? MakeD3D12OwnedResourceHandle(std::move(ownedRecord)) : RhiOwnedResourceHandle{};
}

RhiOwnedResourceHandle D3D12RayTracingServices::CreateRayTracingAccelerationStructureBuffer(
    std::uint64_t sizeInBytes,
    ERhiRayTracingAccelerationStructureType type,
    std::wstring_view debugName)
{
	return CreateAccelerationStructureBuffer(sizeInBytes, type, debugName);
}

RhiOwnedResourceHandle D3D12RayTracingServices::CreateInstanceBuffer(
    const RhiRayTracingInstanceDesc* instances,
    std::uint32_t instanceCount,
    std::wstring_view debugName)
{
	return m_classicTlasServices.CreateClassicTopLevelAccelerationStructureInstanceBuffer(instances, instanceCount, debugName);
}

RhiOwnedResourceHandle D3D12RayTracingServices::CreateRayTracingInstanceBuffer(
    const RhiRayTracingInstanceDesc* instances,
    std::uint32_t instanceCount,
    std::wstring_view debugName)
{
	return CreateInstanceBuffer(instances, instanceCount, debugName);
}

bool D3D12RayTracingServices::BuildPartitionedTopLevelAccelerationStructure(
    ID3D12GraphicsCommandList7* commandList,
    const RhiPartitionedTlasBuildCommandDesc& desc) const noexcept
{
	const RhiRayTracingCapabilities capabilities = GetCapabilities();
	if (!capabilities.Groups.PartitionedTlas.Supported)
	{
		return false;
	}
	return m_nvapiProvider != nullptr && m_nvapiProvider->BuildPartitionedTlas(commandList, desc);
}

std::unique_ptr<RayTracingShaderTable> D3D12RayTracingServices::CreateRayTracingShaderTable(const RayTracingShaderTableDesc& desc)
{
	if (m_rhi == nullptr || !m_rhi->GetRayTracingCapabilities().SupportsRayTracingPipeline)
	{
		throw Diagnostics::Error("D3D12 ray-tracing shader-table creation requires complete pipeline readiness.");
	}
	return std::make_unique<D3D12RayTracingShaderTable>(*m_rhi, desc);
}
