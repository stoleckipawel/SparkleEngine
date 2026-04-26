#include "PCH.h"

#include "Analysis/PsoStatsPass.h"

#include "Core/Public/Files/FileUtils.h"
#include "Inspection/CookedPackageInspection.h"

#include <format>
#include <sstream>

namespace
{
	std::string EscapeCsv(std::string_view value)
	{
		std::string result;
		result.reserve(value.size() + 2);
		result.push_back('"');
		for (const char ch : value)
		{
			if (ch == '"')
			{
				result.push_back('"');
			}
			result.push_back(ch);
		}
		result.push_back('"');
		return result;
	}
}

bool PsoStatsPass::WriteCsv(
    std::span<const CookedShaderPackageOutput> packages,
    const std::filesystem::path& analysisDirectory,
    PsoStatsPassResult& outResult,
    std::string& outErrorMessage)
{
	outResult = {};
	outResult.outputPath = analysisDirectory / "pso-stats.csv";

	std::ostringstream csv;
	csv << "packageId,variantId,packageKey,stage,format,backend,entryPoint,bytecodeSize,resourceBindings,constantBuffers,inputElements,pushConstants,specializationConstants\n";

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
			csv << EscapeCsv(package.packageId) << ','
			    << EscapeCsv(package.variantId) << ','
			    << std::format("{:016X}", package.packageKey) << ','
			    << EscapeCsv(GetShaderStagePrefix(binary.stage)) << ','
			    << EscapeCsv(CookedPackageInspection::GetBinaryFormatName(binary.format)) << ','
			    << EscapeCsv(binary.backendName) << ','
			    << EscapeCsv(binary.entryPoint) << ','
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