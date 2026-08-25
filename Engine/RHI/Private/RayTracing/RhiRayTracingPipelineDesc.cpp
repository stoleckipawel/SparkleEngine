#include "PCH.h"

#include "RayTracing/RhiRayTracingPipelineDesc.h"

RayTracingPipeline::RayTracingPipeline(const RayTracingPipelineDesc& desc) :
    m_generation(desc.Generation)
{
	m_recordContracts.reserve(desc.ShaderExports.size() + desc.HitGroups.size());
	for (const RhiRayTracingShaderExportDesc& shaderExport : desc.ShaderExports)
	{
		m_recordContracts.push_back(
		    RecordContract{
		        .ExportName = std::string(shaderExport.ExportName),
		        .Stage = shaderExport.Shader != nullptr && shaderExport.Shader->Entry != nullptr ? shaderExport.Shader->Entry->Stage
		                                                                                         : ShaderStage::Count,
		        .LocalRecordSizeInBytes = shaderExport.Shader != nullptr && shaderExport.Shader->Entry != nullptr
		            ? shaderExport.Shader->Entry->LocalRecordSizeInBytes
		            : 0,
		        .LocalRecordSignature = shaderExport.Shader != nullptr && shaderExport.Shader->Entry != nullptr
		            ? shaderExport.Shader->Entry->LocalRecordSignature
		            : 0});
	}
	for (const RhiRayTracingHitGroupDesc& hitGroup : desc.HitGroups)
	{
		const RecordContract* closestHit = FindRecordContract(hitGroup.ClosestHitExport);
		m_recordContracts.push_back(
		    RecordContract{
		        .ExportName = std::string(hitGroup.ExportName),
		        .LocalRecordSizeInBytes = closestHit != nullptr ? closestHit->LocalRecordSizeInBytes : 0,
		        .LocalRecordSignature = closestHit != nullptr ? closestHit->LocalRecordSignature : 0,
		        .IsHitGroup = true});
	}
}

RayTracingPipeline::~RayTracingPipeline() noexcept = default;

const RayTracingPipeline::RecordContract* RayTracingPipeline::FindRecordContract(std::string_view exportName) const noexcept
{
	for (const RecordContract& contract : m_recordContracts)
	{
		if (contract.ExportName == exportName)
		{
			return &contract;
		}
	}
	return nullptr;
}

RayTracingShaderTable::RayTracingShaderTable(std::uint64_t generation, std::uint64_t pipelineGeneration) noexcept :
    m_generation(generation),
    m_pipelineGeneration(pipelineGeneration)
{
}

RayTracingShaderTable::~RayTracingShaderTable() noexcept = default;
