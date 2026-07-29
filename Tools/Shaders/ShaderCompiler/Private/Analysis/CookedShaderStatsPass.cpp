#include "PCH.h"

#include "Analysis/CookedShaderStatsPass.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Strings/StringUtils.h"
#include "Inspection/CookedPackageInspection.h"

#include <sstream>

CookedShaderStatsReport CookedShaderStatsPass::WriteCsv(
    std::span<const CookedShaderPackageOutput> packages,
    const std::filesystem::path& analysisDirectory)
{
	CookedShaderStatsReport report;
	report.outputPath = analysisDirectory / "CookedShaderStats.csv";

	std::ostringstream csv;
	csv << "PackageId,ShaderPackageKey,ShaderBlobId,Stage,BinaryFormat,CompilerBackend,CodegenTarget,EntryPoint,ExportName,BytecodeBytes,ResourceBindings,ConstantBuffers,InputElements,PushConstantRanges,SpecializationConstants,PipelineLayoutRecords\n";

	for (const CookedShaderPackageOutput& package : packages)
	{
		InspectedCookedShaderPackage inspectedPackage = CookedPackageInspection::Inspect(package.outputPath);

		for (const InspectedCookedShaderBinary& binary : inspectedPackage.binaries)
		{
			csv << Strings::EscapeCsvField(package.packageId) << ','
			    << Formatting::FormatHexUInt64(package.packageKey) << ','
			    << Formatting::FormatHexUInt64(binary.shaderBlobId) << ','
			    << Strings::EscapeCsvField(GetShaderStagePrefix(binary.stage)) << ','
			    << Strings::EscapeCsvField(CookedPackageInspection::GetBinaryFormatName(binary.format)) << ','
			    << Strings::EscapeCsvField(binary.backendName) << ','
			    << Strings::EscapeCsvField(binary.codegenTarget) << ','
			    << Strings::EscapeCsvField(binary.entryPoint) << ','
			    << Strings::EscapeCsvField(binary.exportName) << ','
			    << binary.bytecodeSizeInBytes << ','
			    << binary.resourceBindingCount << ','
			    << binary.constantBufferCount << ','
			    << binary.inputElementCount << ','
			    << binary.pushConstantRangeCount << ','
			    << binary.specializationConstantCount << ','
			    << inspectedPackage.pipelineLayoutRecordCount << '\n';
			++report.rowCount;
		}
	}

	std::string writeError;
	if (!Files::TryWriteAllText(report.outputPath, csv.str(), writeError))
	{
		throw Diagnostics::Error(std::move(writeError));
	}

	return report;
}
