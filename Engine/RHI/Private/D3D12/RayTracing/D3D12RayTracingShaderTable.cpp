#include "PCH.h"

#include "D3D12/RayTracing/D3D12RayTracingShaderTable.h"

#include "Core/Public/Diagnostics/Error.h"
#include "D3D12/Device/D3D12Rhi.h"
#include "D3D12/Pipeline/D3D12RayTracingPipeline.h"
#include "D3D12/Resources/D3D12UploadBuffer.h"
#include "RayTracing/RhiRayTracingShaderTablePacking.h"
#include "Validation/RhiContract.h"

#include <cstring>
#include <limits>
#include <vector>

std::vector<std::byte> D3D12RayTracingShaderTable::CollectShaderIdentifiers(
    const D3D12RayTracingPipeline& pipeline,
    std::span<const RhiRayTracingShaderRecord> records)
{
	if (records.size() > std::numeric_limits<std::size_t>::max() / D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES)
	{
		throw Diagnostics::Error("D3D12 shader-table identifier storage overflowed.");
	}
	std::vector<std::byte> identifiers(records.size() * D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	for (std::size_t index = 0; index < records.size(); ++index)
	{
		const RhiRayTracingShaderRecord& record = records[index];
		const void* identifier = pipeline.FindShaderIdentifier(record.ExportName);
		if (identifier == nullptr)
		{
			throw Diagnostics::Error("D3D12 shader-table record references an export absent from its pipeline generation.");
		}
		std::byte* const destination = identifiers.data() + index * D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
		std::memcpy(destination, identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	}
	return identifiers;
}

D3D12RayTracingShaderTable::D3D12RayTracingShaderTable(D3D12Rhi& rhi, const RayTracingShaderTableDesc& desc) :
    RayTracingShaderTable(desc.Generation, desc.Pipeline != nullptr ? desc.Pipeline->GetGeneration() : 0)
{
	RhiContract::ValidateRayTracingShaderTableDesc(desc);
	const auto* pipeline = dynamic_cast<const D3D12RayTracingPipeline*>(desc.Pipeline);
	if (pipeline == nullptr)
	{
		throw Diagnostics::Error("D3D12 shader-table creation received a foreign pipeline.");
	}
	const RhiRayTracingShaderTablePackingRules packingRules{
	    .IdentifierSizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES,
	    .RecordAlignmentInBytes = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT,
	    .TableAlignmentInBytes = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT,
	    .MaximumRecordStrideInBytes = D3D12_RAYTRACING_MAX_SHADER_RECORD_STRIDE};
	std::vector<std::byte> bytes;
	m_rayGeneration = RhiRayTracingShaderTablePacking::AppendRegion(
	    desc.RayGenerationRecords,
	    CollectShaderIdentifiers(*pipeline, desc.RayGenerationRecords),
	    packingRules,
	    bytes);
	m_miss = RhiRayTracingShaderTablePacking::AppendRegion(
	    desc.MissRecords,
	    CollectShaderIdentifiers(*pipeline, desc.MissRecords),
	    packingRules,
	    bytes);
	m_hitGroup = RhiRayTracingShaderTablePacking::AppendRegion(
	    desc.HitGroupRecords,
	    CollectShaderIdentifiers(*pipeline, desc.HitGroupRecords),
	    packingRules,
	    bytes);
	m_callable = RhiRayTracingShaderTablePacking::AppendRegion(
	    desc.CallableRecords,
	    CollectShaderIdentifiers(*pipeline, desc.CallableRecords),
	    packingRules,
	    bytes);
	m_allocation = D3D12UploadBuffer::Upload(rhi, bytes.data(), bytes.size());
	if (m_allocation == nullptr || m_allocation->Resource == nullptr)
	{
		throw Diagnostics::Error("D3D12 shader-table allocation failed.");
	}
	if (desc.DebugName != nullptr)
	{
		m_allocation->Resource->SetName(desc.DebugName);
	}
}

D3D12_GPU_VIRTUAL_ADDRESS D3D12RayTracingShaderTable::GetGpuAddress() const noexcept
{
	return m_allocation != nullptr && m_allocation->Resource != nullptr ? m_allocation->Resource->GetGPUVirtualAddress() : 0;
}
