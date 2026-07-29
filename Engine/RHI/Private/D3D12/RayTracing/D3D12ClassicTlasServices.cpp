#include "PCH.h"

#include "D3D12/RayTracing/D3D12ClassicTlasServices.h"

#include "D3D12/D3D12TypeConversions.h"
#include "D3D12/Device/D3D12Rhi.h"
#include "D3D12/Memory/D3D12GpuAllocation.h"
#include "D3D12/Memory/D3D12GpuMemoryAllocator.h"
#include "Validation/RhiContract.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

class D3D12ClassicTlasTranslation final
{
  public:
	static std::wstring MakeDebugName(std::wstring_view debugName, std::wstring_view defaultDebugName)
	{
		return debugName.empty() ? std::wstring(defaultDebugName) : std::wstring(debugName);
	}

	static D3D12_RAYTRACING_INSTANCE_FLAGS ToNativeInstanceFlags(RhiRayTracingInstanceFlags flags) noexcept
	{
		D3D12_RAYTRACING_INSTANCE_FLAGS nativeFlags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		if (HasFlag(flags, RhiRayTracingInstanceFlags::TriangleFacingCullDisable))
		{
			nativeFlags |= D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE;
		}
		if (HasFlag(flags, RhiRayTracingInstanceFlags::ForceOpaque))
		{
			nativeFlags |= D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE;
		}
		if (HasFlag(flags, RhiRayTracingInstanceFlags::ForceNonOpaque))
		{
			nativeFlags |= D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE;
		}
		return nativeFlags;
	}
};

D3D12ClassicTlasServices::D3D12ClassicTlasServices(D3D12Rhi& rhi, D3D12GpuMemoryAllocator& memoryAllocator) noexcept :
    m_rhi(&rhi), m_memoryAllocator(&memoryAllocator)
{
}

RhiRayTracingAccelerationStructurePrebuildInfo D3D12ClassicTlasServices::GetClassicTopLevelAccelerationStructurePrebuildInfo(
    std::uint32_t instanceCount,
    ERhiClassicTlasBuildFlags buildFlags) const noexcept
{
	if (m_rhi == nullptr || !m_rhi->GetRayTracingCapabilities().SupportsRayTracing)
	{
		return {};
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	if (HasFlag(buildFlags, ERhiClassicTlasBuildFlags::AllowUpdate))
	{
		inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	}
	inputs.NumDescs = (std::max) (instanceCount, 1u);

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO nativeInfo{};
	m_rhi->GetDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &nativeInfo);
	RhiRayTracingAccelerationStructurePrebuildInfo prebuildInfo{
	    .ResultDataMaxSizeInBytes = nativeInfo.ResultDataMaxSizeInBytes,
	    .ScratchDataSizeInBytes = nativeInfo.ScratchDataSizeInBytes,
	    .UpdateScratchDataSizeInBytes = nativeInfo.UpdateScratchDataSizeInBytes};
	return prebuildInfo;
}

RhiOwnedResourceHandle D3D12ClassicTlasServices::CreateClassicTopLevelAccelerationStructureInstanceBuffer(
    const RhiRayTracingInstanceDesc* instances,
    std::uint32_t instanceCount,
    std::wstring_view debugName)
{
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || !RhiContract::IsRayTracingInstanceListUsable(instances, instanceCount))
	{
		return {};
	}

	std::vector<D3D12_RAYTRACING_INSTANCE_DESC> nativeInstances((std::max) (instanceCount, 1u));
	for (std::uint32_t instanceIndex = 0; instanceIndex < instanceCount; ++instanceIndex)
	{
		D3D12_RAYTRACING_INSTANCE_DESC& nativeInstance = nativeInstances[instanceIndex];
		const RhiRayTracingInstanceDesc& source = instances[instanceIndex];
		for (std::uint32_t transformIndex = 0; transformIndex < source.Transform.size(); ++transformIndex)
		{
			nativeInstance.Transform[transformIndex / 4][transformIndex % 4] = source.Transform[transformIndex];
		}
		nativeInstance.InstanceID = source.InstanceID;
		nativeInstance.InstanceMask = source.InstanceMask;
		nativeInstance.InstanceContributionToHitGroupIndex = source.InstanceContributionToHitGroupIndex;
		nativeInstance.Flags = D3D12ClassicTlasTranslation::ToNativeInstanceFlags(source.Flags);
		nativeInstance.AccelerationStructure = source.AccelerationStructure;
	}

	const std::uint64_t sizeInBytes = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * static_cast<std::uint64_t>(nativeInstances.size());
	const D3D12_RESOURCE_DESC resourceDesc =
	    D3D12TypeConversions::BuildBufferResourceDesc(RhiBufferResourceDesc{.SizeInBytes = sizeInBytes});
	std::unique_ptr<D3D12GpuAllocationRecord> ownedRecord = m_memoryAllocator->CreateBuffer(
	    resourceDesc,
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    RhiMemoryCategory::RayTracing,
	    RhiMemoryResidencyClass::HostUpload,
	    D3D12ClassicTlasTranslation::MakeDebugName(debugName, L"RayTracingClassicTlasInstanceBuffer"));
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
