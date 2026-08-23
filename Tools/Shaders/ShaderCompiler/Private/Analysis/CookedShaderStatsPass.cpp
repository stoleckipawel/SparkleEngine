#include "PCH.h"

#include "Analysis/CookedShaderStatsPass.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Strings/StringUtils.h"

#include <sstream>

CookedShaderStatsReport CookedShaderStatsPass::WriteCsv(
    const ShaderCookOutput& output,
    const std::filesystem::path& analysisDirectory)
{
	CookedShaderStatsReport report{.outputPath = analysisDirectory / "CookedShaderStats.csv"};
	std::ostringstream csv;
	csv << "ShaderTypeId,ShaderName,Target,ShaderCodeHash,BytecodeBytes\n";
	for (const ShaderCookedEntry& entry : output.entries)
	{
		csv << Formatting::FormatHexUInt64(entry.shaderType) << ',' << Strings::EscapeCsvField(entry.shaderName) << ','
		    << GetShaderTargetName(entry.target) << ',' << Formatting::FormatHexUInt64(entry.codeHash) << ',' << entry.codeSizeInBytes << '\n';
		++report.rowCount;
	}
	std::string error;
	if (!Files::TryWriteAllText(report.outputPath, csv.str(), error))
	{
		throw Diagnostics::Error(std::move(error));
	}
	return report;
}
