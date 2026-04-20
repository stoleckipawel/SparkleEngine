#include "PCH.h"

#include "Cooking/CookedRegistryWriter.h"

#include "Constants/ShaderCompilerConstants.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Manifest/ShaderStageNames.h"
#include "RHI/Public/Shaders/CookedShaderPackageUtils.h"

#include <format>
#include <fstream>

bool CookedRegistryWriter::Write(
	std::span<const CookedShaderPackageOutput> packages,
	std::filesystem::path& outRegistryPath,
	std::string& outErrorMessage)
{
	using Engine::Files::TryOpenTextOutput;
	using Engine::Paths::MakeProjectRelativeString;

	outRegistryPath = ::GetCookedShaderRegistryPath();
	std::ofstream output;
	if (!TryOpenTextOutput(outRegistryPath, output, outErrorMessage))
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
		output << kRegistryKeyPackageKey << " = " << std::format("{:016X}", package.packageKey) << '\n';
		output << kRegistryKeySourceIdentityHash << " = " << std::format("{:016X}", package.sourceIdentityHash) << '\n';
		output << kRegistryKeyBindingLayoutHash << " = " << std::format("{:016X}", package.bindingLayoutHash) << '\n';
		output << kRegistryKeyVariantHash << " = " << std::format("{:016X}", package.variantHash) << '\n';
		output << kRegistryKeyDeclaredStages << " = " << ShaderStageNames::FormatMask(package.declaredStages) << '\n';
		output << kRegistryKeyOutput << " = " << MakeProjectRelativeString(package.outputPath) << "\n\n";
	}

	if (!output.good())
	{
		outErrorMessage = "Failed to write shader registry output '" + outRegistryPath.string() + "'";
		return false;
	}

	outErrorMessage.clear();
	return true;
}
