#include "PCH.h"
#include "Core/Public/FileSystemUtils.h"

#include "Cooking/CookedRegistryWriter.h"

#include "Constants/ShaderCompilerConstants.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Paths/PathUtils.h"
#include "RHI/Public/Shaders/ShaderStage.h"

#include <fstream>

void CookedRegistryWriter::Write(
    std::span<const CookedShaderPackageOutput> packages,
    const std::filesystem::path& storagePath)
{
	const std::filesystem::path tempRegistryPath = Files::BuildTemporaryPath(storagePath);
	std::ofstream output;
	std::string fileError;
	if (!Files::TryOpenTextOutput(tempRegistryPath, output, fileError))
	{
		throw Diagnostics::Error(std::move(fileError));
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
		output.close();
		Files::CleanupTemporaryFile(tempRegistryPath);
		throw Diagnostics::Error("Failed to write shader registry output '" + storagePath.string() + "'");
	}

	if (!Files::TryCloseOutput(output, tempRegistryPath, fileError))
	{
		Files::CleanupTemporaryFile(tempRegistryPath);
		throw Diagnostics::Error(std::move(fileError));
	}

	if (!Files::TryFinalizeTemporaryFile(tempRegistryPath, storagePath, fileError))
	{
		Files::CleanupTemporaryFile(tempRegistryPath);
		throw Diagnostics::Error(std::move(fileError));
	}
}
