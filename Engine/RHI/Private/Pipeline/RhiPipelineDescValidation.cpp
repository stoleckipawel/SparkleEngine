#include "PCH.h"

#include "Pipeline/RhiPipelineDesc.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Validation/RhiContract.h"

#include <cstdint>
#include <limits>
#include <unordered_map>
#include <unordered_set>

namespace RhiPipelineDescValidation
{
	bool HasVertexBinding(const RhiVertexInputDeclaration& declaration, std::uint8_t binding) noexcept
	{
		for (std::uint32_t index = 0; index < declaration.BindingCount; ++index)
		{
			if (declaration.Bindings[index].Binding == binding)
			{
				return true;
			}
		}
		return false;
	}

	void Require(bool condition, const char* message)
	{
		if (!condition)
		{
			throw Diagnostics::Error(message);
		}
	}

	void RequireShaderStage(const RhiShaderStageDesc& shader, ShaderStage expectedStage, const char* message)
	{
		Require(shader.IsValid() && shader.Shader->Entry->Stage == expectedStage, message);
	}
}

void RhiContract::ValidateGraphicsPipelineDesc(const GraphicsPipelineDesc& desc)
{
	RhiPipelineDescValidation::Require(desc.BindingLayout != nullptr, "Graphics pipeline requires a binding layout.");
	RhiPipelineDescValidation::RequireShaderStage(
	    desc.VertexShader,
	    ShaderStage::Vertex,
	    "Graphics pipeline requires a valid vertex shader stage.");
	if (desc.PixelShader.Shader != nullptr)
	{
		RhiPipelineDescValidation::RequireShaderStage(
		    desc.PixelShader,
		    ShaderStage::Pixel,
		    "Graphics pipeline pixel shader descriptor is invalid or has the wrong stage.");
	}
	RhiPipelineDescValidation::Require(
	    desc.ColorAttachmentCount != 0 || desc.DepthStencilAttachmentFormat != PixelFormat::Unknown,
	    "Graphics pipeline requires at least one color or depth-stencil attachment format.");
	RhiPipelineDescValidation::Require(
	    desc.ColorAttachmentCount <= desc.ColorAttachmentFormats.size(),
	    "Graphics pipeline color attachment count exceeds descriptor capacity.");
	RhiPipelineDescValidation::Require(
	    desc.VertexInput.BindingCount <= desc.VertexInput.Bindings.size(),
	    "Graphics pipeline vertex binding count exceeds descriptor capacity.");
	RhiPipelineDescValidation::Require(
	    desc.VertexInput.ElementCount <= desc.VertexInput.Elements.size(),
	    "Graphics pipeline vertex element count exceeds descriptor capacity.");
	RhiPipelineDescValidation::Require(
	    desc.SampleCount == 1 || desc.SampleCount == 2 || desc.SampleCount == 4 || desc.SampleCount == 8,
	    "Graphics pipeline sample count is unsupported by the neutral RHI contract.");
	RhiPipelineDescValidation::Require(
	    desc.Depth.DepthEnable || !desc.Depth.DepthWriteEnable,
	    "Graphics pipeline depth writes require depth testing.");

	for (std::uint32_t index = 0; index < desc.VertexInput.ElementCount; ++index)
	{
		RhiPipelineDescValidation::Require(
		    RhiPipelineDescValidation::HasVertexBinding(desc.VertexInput, desc.VertexInput.Elements[index].Binding),
		    "Graphics pipeline vertex input references a missing binding.");
	}

	const bool hasDepthStencilFormat = desc.DepthStencilAttachmentFormat != PixelFormat::Unknown;
	RhiPipelineDescValidation::Require(
	    (!desc.Depth.DepthEnable && !desc.Depth.DepthWriteEnable && !desc.Stencil.StencilEnable) || hasDepthStencilFormat,
	    "Graphics pipeline depth-stencil state requires an attachment format.");
	RhiPipelineDescValidation::Require(
	    !desc.Stencil.StencilEnable || PixelFormatHasStencilAspect(desc.DepthStencilAttachmentFormat),
	    "Graphics pipeline stencil state requires a stencil-capable attachment format.");

	for (std::uint32_t index = 0; index < desc.ColorAttachmentCount; ++index)
	{
		RhiPipelineDescValidation::Require(
		    IsColorAttachmentPixelFormat(desc.ColorAttachmentFormats[index]),
		    "Graphics pipeline color attachment format is not color-attachment capable.");
	}
	RhiPipelineDescValidation::Require(
	    !hasDepthStencilFormat || IsDepthStencilPixelFormat(desc.DepthStencilAttachmentFormat),
	    "Graphics pipeline depth-stencil attachment format is not depth-stencil capable.");

	const std::uint32_t blendTargetCount = desc.Blend.IndependentBlendEnable ? desc.ColorAttachmentCount : 1;
	for (std::uint32_t index = 0; index < blendTargetCount; ++index)
	{
		RhiPipelineDescValidation::Require(
		    (desc.Blend.Targets[index].ColorWriteMask & 0xF0u) == 0,
		    "Graphics pipeline color write mask contains unsupported bits.");
	}
}

void RhiContract::ValidateComputePipelineDesc(const ComputePipelineDesc& desc)
{
	RhiPipelineDescValidation::Require(desc.BindingLayout != nullptr, "Compute pipeline requires a binding layout.");
	RhiPipelineDescValidation::RequireShaderStage(
	    desc.ComputeShader,
	    ShaderStage::Compute,
	    "Compute pipeline requires a valid compute shader stage.");
}

void RhiContract::ValidateRayTracingPipelineDesc(const RayTracingPipelineDesc& desc)
{
	RhiPipelineDescValidation::Require(desc.GlobalBindingLayout != nullptr, "Ray-tracing pipeline requires a global binding layout.");
	RhiPipelineDescValidation::Require(!desc.ShaderExports.empty(), "Ray-tracing pipeline requires shader exports.");
	RhiPipelineDescValidation::Require(desc.Generation != 0, "Ray-tracing pipeline requires a nonzero generation.");
	RhiPipelineDescValidation::Require(desc.MaxRecursionDepth != 0, "Ray-tracing pipeline recursion depth must be nonzero.");
	RhiPipelineDescValidation::Require(
	    desc.MaxPayloadSizeInBytes != 0 && desc.MaxAttributeSizeInBytes != 0,
	    "Ray-tracing pipeline payload and attribute sizes must be nonzero.");

	std::unordered_map<std::string_view, ShaderStage> exportStages;
	std::uint32_t rayGenerationCount = 0;
	for (const RhiRayTracingShaderExportDesc& shaderExport : desc.ShaderExports)
	{
		RhiPipelineDescValidation::Require(
		    shaderExport.Shader != nullptr && shaderExport.Shader->IsValid() && !shaderExport.ExportName.empty(),
		    "Ray-tracing pipeline contains an invalid shader export.");
		const ShaderStage stage = shaderExport.Shader->Entry->Stage;
		RhiPipelineDescValidation::Require(IsRayTracingShaderStage(stage), "Ray-tracing pipeline contains a non-RT shader stage.");
		RhiPipelineDescValidation::Require(
		    exportStages.emplace(shaderExport.ExportName, stage).second,
		    "Ray-tracing pipeline contains a duplicate shader export.");
		const bool sharedContractMatches = stage != ShaderStage::RayGeneration
		    || (shaderExport.Shader->Entry->RayPayloadSizeInBytes == desc.MaxPayloadSizeInBytes
		        && shaderExport.Shader->Entry->RayAttributeSizeInBytes == desc.MaxAttributeSizeInBytes
		        && shaderExport.Shader->Entry->MinimumRayRecursionDepth <= desc.MaxRecursionDepth);
		RhiPipelineDescValidation::Require(sharedContractMatches, "Ray-tracing pipeline export metadata does not match its map contract.");
		RhiPipelineDescValidation::Require(
		    (shaderExport.Shader->Entry->LocalRecordSizeInBytes == 0) == (shaderExport.Shader->Entry->LocalRecordSignature == 0),
		    "Ray-tracing pipeline export has an incomplete local-record contract.");
		rayGenerationCount += stage == ShaderStage::RayGeneration ? 1u : 0u;
	}
	RhiPipelineDescValidation::Require(rayGenerationCount == 1, "Ray-tracing pipeline requires exactly one ray-generation export.");

	std::unordered_set<std::string_view> hitGroupNames;
	for (const RhiRayTracingHitGroupDesc& group : desc.HitGroups)
	{
		RhiPipelineDescValidation::Require(
		    !group.ExportName.empty() && exportStages.find(group.ExportName) == exportStages.end()
		        && hitGroupNames.insert(group.ExportName).second,
		    "Ray-tracing pipeline contains a missing or duplicate hit-group export.");
		const auto hasStage = [&exportStages](std::string_view name, ShaderStage stage)
		{
			const auto found = exportStages.find(name);
			return found != exportStages.end() && found->second == stage;
		};
		RhiPipelineDescValidation::Require(
		    hasStage(group.ClosestHitExport, ShaderStage::ClosestHit),
		    "Ray-tracing hit group requires a valid closest-hit export.");
		RhiPipelineDescValidation::Require(
		    group.AnyHitExport.empty() || hasStage(group.AnyHitExport, ShaderStage::AnyHit),
		    "Ray-tracing hit group references an invalid any-hit export.");
		if (group.Kind == ERhiRayTracingHitGroupKind::Triangles)
		{
			RhiPipelineDescValidation::Require(
			    group.IntersectionExport.empty(),
			    "Triangle hit group cannot contain an intersection export.");
		}
		else
		{
			RhiPipelineDescValidation::Require(
			    hasStage(group.IntersectionExport, ShaderStage::Intersection),
			    "Procedural hit group requires a valid intersection export.");
		}
	}
}

void RhiContract::ValidateRayTracingShaderTableDesc(const RayTracingShaderTableDesc& desc)
{
	RhiPipelineDescValidation::Require(desc.Pipeline != nullptr, "Ray-tracing shader table requires a pipeline.");
	RhiPipelineDescValidation::Require(desc.Generation != 0, "Ray-tracing shader table requires a nonzero generation.");
	RhiPipelineDescValidation::Require(
	    desc.Pipeline != nullptr && desc.Pipeline->GetGeneration() == desc.Generation,
	    "Ray-tracing shader table generation does not match its pipeline.");
	RhiPipelineDescValidation::Require(
	    desc.RayGenerationRecords.size() == 1,
	    "Ray-tracing shader table requires exactly one ray-generation record.");
	const auto validateRecords =
	    [&desc](std::span<const RhiRayTracingShaderRecord> records, ShaderStage expectedStage, bool expectedHitGroup)
	{
		for (const RhiRayTracingShaderRecord& record : records)
		{
			RhiPipelineDescValidation::Require(!record.ExportName.empty(), "Ray-tracing shader record has no export name.");
			const RayTracingPipeline::RecordContract* contract = desc.Pipeline->FindRecordContract(record.ExportName);
			RhiPipelineDescValidation::Require(
			    contract != nullptr && contract->IsHitGroup == expectedHitGroup && (expectedHitGroup || contract->Stage == expectedStage),
			    "Ray-tracing shader record references an export from the wrong table region.");
			RhiPipelineDescValidation::Require(
			    record.LocalData.size() == contract->LocalRecordSizeInBytes
			        && record.LocalRecordSignature == contract->LocalRecordSignature,
			    "Ray-tracing shader record does not match its local-data contract.");
		}
	};
	validateRecords(desc.RayGenerationRecords, ShaderStage::RayGeneration, false);
	validateRecords(desc.MissRecords, ShaderStage::Miss, false);
	validateRecords(desc.HitGroupRecords, ShaderStage::Count, true);
	validateRecords(desc.CallableRecords, ShaderStage::Callable, false);
}

void RhiContract::ValidateTraceRaysDesc(const TraceRaysDesc& desc, ERhiQueueType queueType)
{
	RhiPipelineDescValidation::Require(queueType == ERhiQueueType::Graphics, "TraceRays requires a graphics queue.");
	RhiPipelineDescValidation::Require(
	    desc.Pipeline != nullptr && desc.ShaderTable != nullptr,
	    "TraceRays requires a pipeline and shader table.");
	RhiPipelineDescValidation::Require(
	    desc.ShaderTable->GetPipelineGeneration() == desc.Pipeline->GetGeneration(),
	    "TraceRays rejected a stale shader table generation.");
	RhiPipelineDescValidation::Require(
	    desc.RayGeneration == desc.ShaderTable->GetRayGenerationRegion() && desc.Miss == desc.ShaderTable->GetMissRegion()
	        && desc.HitGroup == desc.ShaderTable->GetHitGroupRegion() && desc.Callable == desc.ShaderTable->GetCallableRegion(),
	    "TraceRays regions do not match the immutable shader table.");
	RhiPipelineDescValidation::Require(desc.Width != 0 && desc.Height != 0 && desc.Depth != 0, "TraceRays dimensions must be nonzero.");
	const auto validateRegion = [](const RhiRayTracingShaderTableRegion& region, bool required)
	{
		RhiPipelineDescValidation::Require(
		    !required || (region.SizeInBytes != 0 && region.StrideInBytes != 0),
		    "TraceRays required shader-table region is empty.");
		RhiPipelineDescValidation::Require(
		    region.SizeInBytes == 0 || (region.StrideInBytes != 0 && region.SizeInBytes % region.StrideInBytes == 0),
		    "TraceRays shader-table region has invalid size or stride.");
		RhiPipelineDescValidation::Require(
		    region.OffsetInBytes <= std::numeric_limits<std::uint64_t>::max() - region.SizeInBytes,
		    "TraceRays shader-table region arithmetic overflowed.");
	};
	validateRegion(desc.RayGeneration, true);
	validateRegion(desc.Miss, false);
	validateRegion(desc.HitGroup, false);
	validateRegion(desc.Callable, false);
}
