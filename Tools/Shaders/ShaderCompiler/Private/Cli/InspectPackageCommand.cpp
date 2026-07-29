#include "PCH.h"

#include "Cli/InspectPackageCommand.h"

#include "Constants/ShaderCompilerConstants.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Inspection/CookedPackageInspection.h"

#include <iostream>

int InspectPackageCommand::Run(std::span<const std::string_view> args) const
{
	if (args.size() != 1)
	{
		std::cerr << "ShaderCompiler: inspect-package expects <path>\n";
		return kExitCodeUsage;
	}

	InspectedCookedShaderPackage package;
	try
	{
		package = CookedPackageInspection::Inspect(std::filesystem::path(std::string(args[0])));
	}
	catch (const Diagnostics::Error& error)
	{
		std::cerr << "ShaderCompiler: failed to inspect package - " << error.what() << "\n";
		return kExitCodeUsage;
	}

	std::cout << "Package key=" << Formatting::FormatPrefixedHexUInt64(package.packageKey)
	          << " kind=" << CookedPackageInspection::GetPackageKindName(package.packageKind)
	          << " features='" << CookedPackageInspection::FormatPackageFeatures(package.packageFeatures) << "'"
	          << " binaries=" << package.binaryRecordCount
	          << " pipelineLayouts=" << package.pipelineLayoutRecordCount
	          << " reflections=" << package.reflectionRecordCount
	          << " rtExports=" << package.rayTracingExports.size()
	          << " rtHitGroups=" << package.rayTracingHitGroups.size()
	          << " localParameters=" << package.rayTracingLocalParameterRecordCount << "\n";
	std::cout << "  hashes source=" << Formatting::FormatPrefixedHexUInt64(package.sourceIdentityHash)
	          << " layout=" << Formatting::FormatPrefixedHexUInt64(package.bindingLayoutHash) << "\n";
	if (package.packageKind == CookedShaderPackageKind::RayTracingLibrary)
	{
		std::cout << "  rt payloadBytes=" << package.rayTracingPayloadSizeInBytes
		          << " attributeBytes=" << package.rayTracingAttributeSizeInBytes
		          << " maxRecursion=" << package.rayTracingMaxRecursionDepth << "\n";
	}
	std::cout << "Bytecode:\n";
	for (const InspectedCookedShaderBinary& binary : package.binaries)
	{
		std::cout << "  ShaderBlobId=" << Formatting::FormatPrefixedHexUInt64(binary.shaderBlobId)
		          << " Stage=" << GetShaderStagePrefix(binary.stage)
		          << " BinaryFormat=" << CookedPackageInspection::GetBinaryFormatName(binary.format)
		          << " EntryPoint=" << binary.entryPoint
		          << " ExportName=" << binary.exportName
		          << " CompilerBackend=" << binary.backendName
		          << " CodegenTarget=" << binary.codegenTarget
		          << " backendVersion=" << Formatting::FormatPrefixedHexUInt64(binary.backendVersion)
		          << " BytecodeHash=" << Formatting::FormatPrefixedHexUInt64(binary.bytecodeHash)
		          << " BytecodeBytes=" << binary.bytecodeSizeInBytes << "\n";
	}
	std::cout << "Reflection:\n";
	for (const InspectedCookedShaderBinary& binary : package.binaries)
	{
		std::cout << "  Stage=" << GetShaderStagePrefix(binary.stage)
		          << " BinaryFormat=" << CookedPackageInspection::GetBinaryFormatName(binary.format)
		          << " ResourceBindings=" << binary.resourceBindingCount
		          << " ConstantBuffers=" << binary.constantBufferCount
		          << " InputElements=" << binary.inputElementCount
		          << " PushConstantRanges=" << binary.pushConstantRangeCount
		          << " SpecializationConstants=" << binary.specializationConstantCount << "\n";
	}
	std::cout << "PipelineLayout:\n";
	for (const InspectedCookedPipelineLayout& layout : package.pipelineLayouts)
	{
		std::cout << "  CodegenTarget=" << layout.codegenTarget
		          << " BindingLayoutHash=" << Formatting::FormatPrefixedHexUInt64(layout.bindingLayoutHash)
		          << " BindingRecords=" << layout.bindingRecordCount
		          << " DescriptorSets=" << layout.descriptorSetCount
		          << " DescriptorBindings=" << layout.descriptorBindingCount
		          << " PushConstantRanges=" << layout.pushConstantRangeCount
		          << " PushConstantBytes=" << layout.pushConstantSizeInBytes
		          << " ConstantBuffers=" << layout.constantBufferCount
		          << " ReadOnlyResources=" << layout.readOnlyResourceCount
		          << " ReadWriteResources=" << layout.readWriteResourceCount
		          << " Samplers=" << layout.samplerCount
		          << " AccelerationStructures=" << layout.accelerationStructureCount << "\n";
	}
	for (const InspectedCookedRayTracingExport& rtExport : package.rayTracingExports)
	{
		std::cout << "  export name=" << rtExport.exportName
		          << " kind=" << CookedPackageInspection::GetRayTracingExportKindName(rtExport.kind)
		          << " entry=" << rtExport.entryPoint
		          << " binary=" << rtExport.binaryRecordIndex
		          << " hash=" << Formatting::FormatPrefixedHexUInt64(rtExport.exportHash) << "\n";
	}
	for (const InspectedCookedRayTracingHitGroup& hitGroup : package.rayTracingHitGroups)
	{
		std::cout << "  hitGroup name=" << hitGroup.name
		          << " type=" << CookedPackageInspection::GetRayTracingHitGroupTypeName(hitGroup.type)
		          << " closest=" << hitGroup.closestHitExportIndex
		          << " any=" << hitGroup.anyHitExportIndex
		          << " intersection=" << hitGroup.intersectionExportIndex
		          << " hash=" << Formatting::FormatPrefixedHexUInt64(hitGroup.hitGroupHash) << "\n";
	}
	for (const InspectedCookedAccelerationStructureBinding& binding : package.accelerationStructureBindings)
	{
		std::cout << "  accelerationStructure name=" << binding.name
		          << " set=" << binding.set
		          << " slot=" << binding.slot
		          << " array=" << binding.arrayCount << "\n";
	}
	return kExitCodeSuccess;
}
