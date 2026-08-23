#include "PCH.h"

#include "Inspection/CookedPackageInspection.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Files/BinarySpanReader.h"
#include "Core/Public/Files/FileUtils.h"

#include <span>

class CookedPackageStringResolver final
{
  public:
	static std::string_view ResolveString(CookedShaderStringRef ref, std::span<const std::uint8_t> stringTable) noexcept
	{
		if (!ref || ref.OffsetInBytes + ref.SizeInBytes > stringTable.size())
		{
			return {};
		}

		return std::string_view(reinterpret_cast<const char*>(stringTable.data() + ref.OffsetInBytes), ref.SizeInBytes);
	}
};

InspectedCookedShaderPackage CookedPackageInspection::Inspect(const std::filesystem::path& packagePath)
{
	std::vector<std::uint8_t> bytes;
	std::string readError;
	if (!Files::TryReadAllBytes(packagePath, bytes, readError))
	{
		throw Diagnostics::Error(std::move(readError));
	}

	if (bytes.size() < sizeof(CookedShaderPackageHeader))
	{
		throw Diagnostics::Error("Cooked shader package is too small");
	}

	Files::BinarySpanReader reader(bytes);
	CookedShaderPackageHeader header;
	if (!reader.ReadValue(header, readError))
	{
		throw Diagnostics::Error("Cooked shader package is too small");
	}
	if (!header.Matches(kCookedShaderPackageMagic, kCookedShaderPackageVersion))
	{
		throw Diagnostics::Error("Unsupported cooked shader package header");
	}

	std::span<const CookedShaderBinaryRecord> binaries;
	std::span<const CookedShaderPipelineLayoutRecord> pipelineLayouts;
	std::span<const CookedShaderReflectionRecord> reflections;
	std::span<const CookedShaderResourceBindingRecord> resourceBindings;
	std::span<const CookedShaderRayTracingExportRecord> rtExports;
	std::span<const CookedShaderRayTracingHitGroupRecord> rtHitGroups;
	if (!reader.ReadArrayView(header.BinaryRecordCount, binaries, readError) ||
	    !reader.SkipArray<CookedShaderBindingRecord>(header.BindingRecordCount, readError) ||
	    !reader.ReadArrayView(header.PipelineLayoutRecordCount, pipelineLayouts, readError) ||
	    !reader.SkipArray<CookedShaderSpecializationInputRecord>(header.SpecializationInputCount, readError) ||
	    !reader.ReadArrayView(header.ReflectionRecordCount, reflections, readError) ||
	    !reader.ReadArrayView(header.ResourceBindingRecordCount, resourceBindings, readError) ||
	    !reader.SkipArray<CookedShaderConstantBufferRecord>(header.ConstantBufferRecordCount, readError) ||
	    !reader.SkipArray<CookedShaderConstantBufferMemberRecord>(header.ConstantBufferMemberRecordCount, readError) ||
	    !reader.SkipArray<CookedShaderInputElementRecord>(header.InputElementRecordCount, readError) ||
	    !reader.SkipArray<CookedShaderPushConstantRangeRecord>(header.PushConstantRangeRecordCount, readError) ||
	    !reader.SkipArray<CookedShaderSpecializationConstantRecord>(header.SpecializationConstantRecordCount, readError) ||
	    !reader.ReadArrayView(header.RayTracingExportRecordCount, rtExports, readError) ||
	    !reader.ReadArrayView(header.RayTracingHitGroupRecordCount, rtHitGroups, readError) ||
	    !reader.SkipArray<CookedShaderRayTracingLocalParameterRecord>(header.RayTracingLocalParameterRecordCount, readError))
	{
		throw Diagnostics::Error("Cooked shader package arrays are truncated");
	}

	std::span<const std::uint8_t> stringTable;
	if (!reader.ReadBytes(header.StringTableSizeInBytes, stringTable, readError))
	{
		throw Diagnostics::Error("Cooked shader package string table is truncated");
	}

	InspectedCookedShaderPackage outPackage;
	outPackage.packageKey = header.ShaderPackageKey;
	outPackage.sourceIdentityHash = header.SourceIdentityHash;
	outPackage.bindingLayoutHash = header.BindingLayoutHash;
	outPackage.packageKind = header.PackageKind;
	outPackage.packageFeatures = header.PackageFeatures;
	outPackage.rayTracingPayloadSizeInBytes = header.RayTracingPayloadSizeInBytes;
	outPackage.rayTracingAttributeSizeInBytes = header.RayTracingAttributeSizeInBytes;
	outPackage.rayTracingMaxRecursionDepth = header.RayTracingMaxRecursionDepth;
	outPackage.binaryRecordCount = header.BinaryRecordCount;
	outPackage.pipelineLayoutRecordCount = header.PipelineLayoutRecordCount;
	outPackage.reflectionRecordCount = header.ReflectionRecordCount;
	outPackage.rayTracingLocalParameterRecordCount = header.RayTracingLocalParameterRecordCount;
	outPackage.binaries.reserve(binaries.size());

	for (std::size_t index = 0; index < binaries.size(); ++index)
	{
		const CookedShaderBinaryRecord& binary = binaries[index];
		InspectedCookedShaderBinary inspectedBinary{};
		inspectedBinary.shaderBlobId = binary.ShaderBlobId;
		inspectedBinary.stage = binary.Stage;
		inspectedBinary.format = binary.Format;
		inspectedBinary.entryPoint = std::string(CookedPackageStringResolver::ResolveString(binary.EntryPoint, stringTable));
		inspectedBinary.exportName = std::string(CookedPackageStringResolver::ResolveString(binary.ExportName, stringTable));
		inspectedBinary.backendName = std::string(CookedPackageStringResolver::ResolveString(binary.BackendName, stringTable));
		inspectedBinary.codegenTarget = std::string(CookedPackageStringResolver::ResolveString(binary.CodegenTarget, stringTable));
		inspectedBinary.bytecodeHash = binary.BytecodeHash;
		inspectedBinary.backendVersion = binary.BackendVersion;
		inspectedBinary.bytecodeSizeInBytes = binary.Bytecode.SizeInBytes;
		if (index < reflections.size())
		{
			const CookedShaderReflectionRecord& reflection = reflections[index];
			inspectedBinary.resourceBindingCount = reflection.ResourceBindingCount;
			inspectedBinary.constantBufferCount = reflection.ConstantBufferCount;
			inspectedBinary.inputElementCount = reflection.InputElementCount;
			inspectedBinary.pushConstantRangeCount = reflection.PushConstantRangeCount;
			inspectedBinary.specializationConstantCount = reflection.SpecializationConstantCount;
		}
		outPackage.binaries.push_back(std::move(inspectedBinary));
	}

	outPackage.pipelineLayouts.reserve(pipelineLayouts.size());
	for (const CookedShaderPipelineLayoutRecord& layout : pipelineLayouts)
	{
		outPackage.pipelineLayouts.push_back(InspectedCookedPipelineLayout{
		    .codegenTarget = std::string(CookedPackageStringResolver::ResolveString(layout.CodegenTarget, stringTable)),
		    .bindingLayoutHash = layout.BindingLayoutHash,
		    .bindingRecordCount = layout.BindingRecordCount,
		    .descriptorBindingCount = layout.DescriptorBindingCount,
		    .descriptorSetCount = layout.DescriptorSetCount,
		    .pushConstantRangeCount = layout.PushConstantRangeCount,
		    .pushConstantSizeInBytes = layout.PushConstantSizeInBytes,
		    .constantBufferCount = layout.ConstantBufferCount,
		    .readOnlyResourceCount = layout.ReadOnlyResourceCount,
		    .readWriteResourceCount = layout.ReadWriteResourceCount,
		    .samplerCount = layout.SamplerCount,
		    .accelerationStructureCount = layout.AccelerationStructureCount});
	}

	outPackage.rayTracingExports.reserve(rtExports.size());
	for (const CookedShaderRayTracingExportRecord& rtExport : rtExports)
	{
		outPackage.rayTracingExports.push_back(InspectedCookedRayTracingExport{
		    .kind = rtExport.Kind,
		    .exportName = std::string(CookedPackageStringResolver::ResolveString(rtExport.ExportName, stringTable)),
		    .entryPoint = std::string(CookedPackageStringResolver::ResolveString(rtExport.EntryPoint, stringTable)),
		    .binaryRecordIndex = rtExport.BinaryRecordIndex,
		    .exportHash = rtExport.ExportHash});
	}

	outPackage.rayTracingHitGroups.reserve(rtHitGroups.size());
	for (const CookedShaderRayTracingHitGroupRecord& hitGroup : rtHitGroups)
	{
		outPackage.rayTracingHitGroups.push_back(InspectedCookedRayTracingHitGroup{
		    .type = hitGroup.Type,
		    .name = std::string(CookedPackageStringResolver::ResolveString(hitGroup.HitGroupName, stringTable)),
		    .closestHitExportIndex = hitGroup.ClosestHitExportIndex,
		    .anyHitExportIndex = hitGroup.AnyHitExportIndex,
		    .intersectionExportIndex = hitGroup.IntersectionExportIndex,
		    .hitGroupHash = hitGroup.HitGroupHash});
	}

	outPackage.accelerationStructureBindings.reserve(resourceBindings.size());
	for (const CookedShaderResourceBindingRecord& binding : resourceBindings)
	{
		if (binding.Kind != CookedShaderResourceKind::AccelerationStructure)
		{
			continue;
		}
		const CookedShaderStringRef nameRef{binding.NameOffsetInBytes, binding.NameSizeInBytes};
		outPackage.accelerationStructureBindings.push_back(InspectedCookedAccelerationStructureBinding{
		    .name = std::string(CookedPackageStringResolver::ResolveString(nameRef, stringTable)),
		    .set = binding.Set,
		    .slot = binding.Slot,
		    .arrayCount = binding.ArrayCount});
	}

	return outPackage;
}

const char* CookedPackageInspection::GetBinaryFormatName(CookedShaderBinaryFormat format) noexcept
{
	switch (format)
	{
		case CookedShaderBinaryFormat::Dxil:
			return "dxil";
		case CookedShaderBinaryFormat::SpirV:
			return "spirv";
		default:
			return "unknown";
	}
}

const char* CookedPackageInspection::GetPackageKindName(CookedShaderPackageKind kind) noexcept
{
	switch (kind)
	{
		case CookedShaderPackageKind::Graphics:
			return "graphics";
		case CookedShaderPackageKind::Compute:
			return "compute";
		case CookedShaderPackageKind::RayTracingLibrary:
			return "ray-tracing-library";
		default:
			return "unknown";
	}
}

const char* CookedPackageInspection::GetRayTracingExportKindName(CookedShaderRayTracingExportKind kind) noexcept
{
	switch (kind)
	{
		case CookedShaderRayTracingExportKind::RayGeneration:
			return "ray-generation";
		case CookedShaderRayTracingExportKind::Miss:
			return "miss";
		case CookedShaderRayTracingExportKind::ClosestHit:
			return "closest-hit";
		case CookedShaderRayTracingExportKind::AnyHit:
			return "any-hit";
		case CookedShaderRayTracingExportKind::Intersection:
			return "intersection";
		case CookedShaderRayTracingExportKind::Callable:
			return "callable";
		case CookedShaderRayTracingExportKind::None:
		default:
			return "none";
	}
}

const char* CookedPackageInspection::GetRayTracingHitGroupTypeName(CookedShaderRayTracingHitGroupType type) noexcept
{
	switch (type)
	{
		case CookedShaderRayTracingHitGroupType::Triangles:
			return "triangles";
		case CookedShaderRayTracingHitGroupType::ProceduralPrimitive:
			return "procedural-primitive";
		default:
			return "unknown";
	}
}

std::string CookedPackageInspection::FormatPackageFeatures(CookedShaderPackageFeatureFlags features)
{
	std::string output;
	if (HasCookedShaderPackageFeature(features, CookedShaderPackageFeatureFlags::UsesInlineRayQuery))
	{
		output = "inline-ray-query";
	}
	if (HasCookedShaderPackageFeature(features, CookedShaderPackageFeatureFlags::UsesAccelerationStructure))
	{
		if (!output.empty())
		{
			output += ", ";
		}
		output += "acceleration-structure";
	}
	if (HasCookedShaderPackageFeature(features, CookedShaderPackageFeatureFlags::UsesDescriptorIndexing))
	{
		if (!output.empty())
		{
			output += ", ";
		}
		output += "descriptor-indexing";
	}
	if (output.empty())
	{
		output = "none";
	}
	return output;
}
