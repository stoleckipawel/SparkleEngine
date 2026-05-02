#include "PCH.h"

#include "Cooking/CookedRegistryWriter.h"

#include "Constants/ShaderCompilerConstants.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Paths/PathUtils.h"
#include "RHI/Public/Shaders/ShaderStage.h"

#include <fstream>

bool CookedRegistryWriter::Write(
	std::span<const CookedShaderPackageOutput> packages,
	std::filesystem::path& outRegistryPath,
	std::string& outErrorMessage)
{
	using Files::TryOpenTextOutput;
	using Files::BuildTemporaryPath;
	using Files::TryCloseOutput;
	using Files::TryFinalizeTemporaryFile;
	using Paths::MakeProjectRelativeString;

	outRegistryPath = Paths::CookedShaderRegistry();
	const std::filesystem::path tempRegistryPath = BuildTemporaryPath(outRegistryPath);
	std::ofstream output;
	if (!TryOpenTextOutput(tempRegistryPath, output, outErrorMessage))
	{
		return false;
	}

	output << kRegistryHeaderSection << '\n';
	output << kRegistryKeyVersion << " = " << kRegistryFormatVersion << '\n';
	output << kRegistryKeyPackageCount << " = " << packages.size() << "\n\n";

	for (const CookedShaderPackageOutput& package : packages)
	{
		output << '[' << kRegistryPackageSectionPrefix << package.packageId << "]\n";
		output << kRegistryKeyVariant << " = " << package.variantId << '\n';
		output << kRegistryKeyBindingLayout << " = " << package.bindingLayoutId << '\n';
		output << kRegistryKeyPackageKey << " = " << Formatting::FormatHexUInt64(package.packageKey) << '\n';
		output << kRegistryKeySourceIdentityHash << " = " << Formatting::FormatHexUInt64(package.sourceIdentityHash) << '\n';
		output << kRegistryKeyBindingLayoutHash << " = " << Formatting::FormatHexUInt64(package.bindingLayoutHash) << '\n';
		output << kRegistryKeyVariantHash << " = " << Formatting::FormatHexUInt64(package.variantHash) << '\n';
		output << kRegistryKeyDeclaredStages << " = " << FormatShaderStageMask(package.declaredStages) << '\n';
		output << kRegistryKeyOutput << " = " << MakeProjectRelativeString(package.outputPath) << "\n\n";
	}

	if (!output.good())
	{
		outErrorMessage = "Failed to write shader registry output '" + outRegistryPath.string() + "'";
		return false;
	}

	if (!TryCloseOutput(output, tempRegistryPath, outErrorMessage))
	{
		return false;
	}

	if (!TryFinalizeTemporaryFile(tempRegistryPath, outRegistryPath, outErrorMessage))
	{
		return false;
	}

	outErrorMessage.clear();
	return true;
}
