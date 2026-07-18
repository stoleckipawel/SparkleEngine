#include "PCH.h"
#include "Core/Public/FileSystemUtils.h"

#include "Cooking/CookedRegistryWriter.h"

#include "Constants/ShaderCompilerConstants.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Core/Public/Paths/PathUtils.h"
#include "RHI/Public/Shaders/ShaderStage.h"

#include <fstream>

bool CookedRegistryWriter::Write(
    std::span<const CookedShaderPackageOutput> packages,
    const std::filesystem::path& storagePath,
    std::string& outErrorMessage)
{
	const std::filesystem::path tempRegistryPath = Files::BuildTemporaryPath(storagePath);
	std::ofstream output;
	if (!Files::TryOpenTextOutput(tempRegistryPath, output, outErrorMessage))
	{
		return false;
	}

	output << kRegistryHeaderSection << '\n';
	output << kRegistryKeyVersion << " = " << kRegistryFormatVersion << '\n';
	output << kRegistryKeyPackageCount << " = " << packages.size() << "\n\n";

	for (const CookedShaderPackageOutput& package : packages)
	{
		output << '[' << kRegistryPackageSectionPrefix << package.packageId << "]\n";
		output << kRegistryKeyBindingLayout << " = " << package.bindingLayoutId << '\n';
		output << kRegistryKeyPackageKey << " = " << Formatting::FormatHexUInt64(package.packageKey) << '\n';
		output << kRegistryKeySourceIdentityHash << " = " << Formatting::FormatHexUInt64(package.sourceIdentityHash) << '\n';
		output << kRegistryKeyBindingLayoutHash << " = " << Formatting::FormatHexUInt64(package.bindingLayoutHash) << '\n';
		output << kRegistryKeyDeclaredStages << " = " << FormatShaderStageMask(package.declaredStages) << '\n';
		output << kRegistryKeyOutput << " = " << Paths::MakeProjectRelativeString(package.outputPath) << "\n\n";
	}

	if (!output.good())
	{
		outErrorMessage = "Failed to write shader registry output '" + storagePath.string() + "'";
		return false;
	}

	if (!Files::TryCloseOutput(output, tempRegistryPath, outErrorMessage))
	{
		return false;
	}

	if (!Files::TryFinalizeTemporaryFile(tempRegistryPath, storagePath, outErrorMessage))
	{
		return false;
	}

	outErrorMessage.clear();
	return true;
}
