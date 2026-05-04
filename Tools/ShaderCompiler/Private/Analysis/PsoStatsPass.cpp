#include "PCH.h"

#include "Analysis/PsoStatsPass.h"

#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Strings/StringUtils.h"
#include "Inspection/CookedPackageInspection.h"

#include <sstream>

bool PsoStatsPass::WriteCsv(
    std::span<const CookedShaderPackageOutput> packages,
    const std::filesystem::path& analysisDirectory,
    PsoStatsPassResult& outResult,
    std::string& outErrorMessage)
{
	outResult = {};
	outResult.outputPath = analysisDirectory / "pso-stats.csv";

	std::ostringstream csv;
	csv << "packageId,packageKey,stage,format,backend,entryPoint,bytecodeSize,resourceBindings,constantBuffers,inputElements,pushConstants,specializationConstants\n";

	for (const CookedShaderPackageOutput& package : packages)
	{
		InspectedCookedShaderPackage inspectedPackage;
		if (!CookedPackageInspection::Inspect(package.outputPath, inspectedPackage, outErrorMessage))
		{
			outErrorMessage = "Failed to inspect cooked package for pso-stats '" + package.outputPath.string() + "' - " + outErrorMessage;
			return false;
		}

		for (const InspectedCookedShaderBinary& binary : inspectedPackage.binaries)
		{
			csv << Strings::EscapeCsvField(package.packageId) << ','
			    << Formatting::FormatHexUInt64(package.packageKey) << ','
			    << Strings::EscapeCsvField(GetShaderStagePrefix(binary.stage)) << ','
			    << Strings::EscapeCsvField(CookedPackageInspection::GetBinaryFormatName(binary.format)) << ','
			    << Strings::EscapeCsvField(binary.backendName) << ','
			    << Strings::EscapeCsvField(binary.entryPoint) << ','
			    << binary.bytecodeSizeInBytes << ','
			    << binary.resourceBindingCount << ','
			    << binary.constantBufferCount << ','
			    << binary.inputElementCount << ','
			    << binary.pushConstantRangeCount << ','
			    << binary.specializationConstantCount << '\n';
			++outResult.rowCount;
		}
	}

	if (!Files::TryWriteAllText(outResult.outputPath, csv.str(), outErrorMessage))
	{
		return false;
	}

	outErrorMessage.clear();
	return true;
}