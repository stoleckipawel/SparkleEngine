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
	          << " binaries=" << package.binaryRecordCount
	          << " reflections=" << package.reflectionRecordCount << "\n";
	for (const InspectedCookedShaderBinary& binary : package.binaries)
	{
		std::cout << "  " << GetShaderStagePrefix(binary.stage)
		          << " format=" << CookedPackageInspection::GetBinaryFormatName(binary.format)
		          << " entry=" << binary.entryPoint
		          << " backend=" << binary.backendName
		          << " bytecode=" << binary.bytecodeSizeInBytes
		          << " resources=" << binary.resourceBindingCount
		          << " cbuffers=" << binary.constantBufferCount
		          << " inputs=" << binary.inputElementCount << "\n";
	}
	return kExitCodeSuccess;
}