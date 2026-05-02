#include "PCH.h"

#include "Cli/InspectPackageCommand.h"

#include "Constants/ShaderCompilerConstants.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Inspection/CookedPackageInspection.h"

#include <iostream>

int InspectPackageCommand::Run(std::span<const std::string_view> args) const
{
	if (args.size() != 1)
	{
		std::cerr << "ShaderCompiler: inspect-package requires <path>\n";
		return kExitCodeUsage;
	}

	InspectedCookedShaderPackage package;
	std::string errorMessage;
	if (!CookedPackageInspection::Inspect(std::filesystem::path(std::string(args[0])), package, errorMessage))
	{
		std::cerr << "ShaderCompiler: failed to inspect package - " << errorMessage << "\n";
		return kExitCodeUsage;
	}

	std::cout << "Package key=" << Formatting::FormatPrefixedHexUInt64(package.packageKey)
	          << " kind=" << CookedPackageInspection::GetPackageKindName(package.packageKind)
	          << " features='" << CookedPackageInspection::FormatPackageFeatures(package.packageFeatures) << "'"
	          << " binaries=" << package.binaryRecordCount
	          << " reflections=" << package.reflectionRecordCount
	          << " rtExports=" << package.rayTracingExports.size()
	          << " rtHitGroups=" << package.rayTracingHitGroups.size()
	          << " localParameters=" << package.rayTracingLocalParameterRecordCount << "\n";
	std::cout << "  hashes source=" << Formatting::FormatPrefixedHexUInt64(package.sourceIdentityHash)
	          << " layout=" << Formatting::FormatPrefixedHexUInt64(package.bindingLayoutHash)
	          << " variant=" << Formatting::FormatPrefixedHexUInt64(package.variantHash) << "\n";
	if (package.packageKind == CookedShaderPackageKind::RayTracingLibrary)
	{
		std::cout << "  rt payloadBytes=" << package.rayTracingPayloadSizeInBytes
		          << " attributeBytes=" << package.rayTracingAttributeSizeInBytes
		          << " maxRecursion=" << package.rayTracingMaxRecursionDepth << "\n";
	}
	for (const InspectedCookedShaderBinary& binary : package.binaries)
	{
		std::cout << "  " << GetShaderStagePrefix(binary.stage)
		          << " format=" << CookedPackageInspection::GetBinaryFormatName(binary.format)
		          << " entry=" << binary.entryPoint
		          << " backend=" << binary.backendName
		          << " backendVersion=" << Formatting::FormatPrefixedHexUInt64(binary.backendVersion)
		          << " bytecodeHash=" << Formatting::FormatPrefixedHexUInt64(binary.bytecodeHash)
		          << " bytecode=" << binary.bytecodeSizeInBytes
		          << " resources=" << binary.resourceBindingCount
		          << " cbuffers=" << binary.constantBufferCount
		          << " inputs=" << binary.inputElementCount << "\n";
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