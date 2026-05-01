#include "PCH.h"

#include "Cli/InspectPackageCommand.h"

#include "Constants/ShaderCompilerConstants.h"
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

	std::cout << "Package key=0x" << std::hex << package.packageKey << std::dec
	          << " kind=" << CookedPackageInspection::GetPackageKindName(package.packageKind)
	          << " features='" << CookedPackageInspection::FormatPackageFeatures(package.packageFeatures) << "'"
	          << " binaries=" << package.binaryRecordCount
	          << " reflections=" << package.reflectionRecordCount
	          << " rtExports=" << package.rayTracingExports.size()
	          << " rtHitGroups=" << package.rayTracingHitGroups.size()
	          << " localParameters=" << package.rayTracingLocalParameterRecordCount << "\n";
	std::cout << "  hashes source=0x" << std::hex << package.sourceIdentityHash
	          << " layout=0x" << package.bindingLayoutHash
	          << " variant=0x" << package.variantHash << std::dec << "\n";
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
		          << " backendVersion=0x" << std::hex << binary.backendVersion
		          << " bytecodeHash=0x" << binary.bytecodeHash << std::dec
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
		          << " hash=0x" << std::hex << rtExport.exportHash << std::dec << "\n";
	}
	for (const InspectedCookedRayTracingHitGroup& hitGroup : package.rayTracingHitGroups)
	{
		std::cout << "  hitGroup name=" << hitGroup.name
		          << " type=" << CookedPackageInspection::GetRayTracingHitGroupTypeName(hitGroup.type)
		          << " closest=" << hitGroup.closestHitExportIndex
		          << " any=" << hitGroup.anyHitExportIndex
		          << " intersection=" << hitGroup.intersectionExportIndex
		          << " hash=0x" << std::hex << hitGroup.hitGroupHash << std::dec << "\n";
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