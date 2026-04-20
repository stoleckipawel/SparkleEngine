#include "PCH.h"

#include "Cooking/CookedPackageWriter.h"

#include "Constants/ShaderCompilerConstants.h"
#include "Cooking/BindingRecordBuilder.h"
#include "Cooking/SourceIdentityHasher.h"
#include "Core/Public/Files/BinaryStreamWriter.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Strings/StringTableBuilder.h"

#include "RHI/Public/Config/RenderConfig.h"
#include "RHI/Public/Shaders/CookedShaderPackageUtils.h"

#include <fstream>

static CookedShaderStringRef ToCookedShaderStringRef(const Engine::Strings::StringTableEntry& entry)
{
	return CookedShaderStringRef{entry.OffsetInBytes, entry.SizeInBytes};
}

bool CookedPackageWriter::Write(
	const ShaderCookPackageDesc& package,
	const PassParameterLayout& bindingLayout,
	std::span<const CookedStageBuild> compiledStages,
	CookedShaderPackageOutput& outPackageOutput,
	std::string& outErrorMessage)
{
	using Engine::Files::BinaryStreamWriter;
	using Engine::Files::TryOpenBinaryOutput;

	Engine::Strings::StringTableBuilder stringTable;
	std::vector<CookedShaderBinaryRecord> binaryRecords;
	std::vector<CookedShaderBindingRecord> bindingRecords;
	std::vector<CookedShaderSpecializationInputRecord> specializationInputs;
	std::vector<std::uint8_t> binaryBlob;

	BindingRecordBuilder::Build(bindingLayout, stringTable, bindingRecords);

	binaryRecords.reserve(compiledStages.size());
	binaryBlob.reserve(kBinaryBlobInitialReserveBytes);
	ShaderStageMask declaredStages = ShaderStageMask::None;

	for (const CookedStageBuild& compiledStage : compiledStages)
	{
		const std::uint32_t blobOffset = static_cast<std::uint32_t>(binaryBlob.size());
		binaryBlob.insert(binaryBlob.end(), compiledStage.bytecode.begin(), compiledStage.bytecode.end());

		binaryRecords.push_back(
		    CookedShaderBinaryRecord{
		        .EntryPoint = ToCookedShaderStringRef(stringTable.Add(compiledStage.entryPoint)),
		        .DebugArtifact = ToCookedShaderStringRef(stringTable.Add(compiledStage.debugArtifact)),
		        .Bytecode = CookedShaderBlobRef{blobOffset, static_cast<std::uint32_t>(compiledStage.bytecode.size())},
		        .Stage = compiledStage.stage,
		        .Format = CookedShaderBinaryFormat::Dxil,
		        .BytecodeHash = compiledStage.bytecodeHash});

		declaredStages |= ToShaderStageMask(compiledStage.stage);
	}

	CookedShaderPackageHeader header{};
	header.DeclaredStages = declaredStages;
	header.ShaderModelMajor = static_cast<std::uint16_t>(RenderConfig::ShaderModelMajor);
	header.ShaderModelMinor = static_cast<std::uint16_t>(RenderConfig::ShaderModelMinor);
	header.BinaryRecordCount = static_cast<std::uint32_t>(binaryRecords.size());
	header.BindingRecordCount = static_cast<std::uint32_t>(bindingRecords.size());
	header.SpecializationInputCount = static_cast<std::uint32_t>(specializationInputs.size());
	header.StringTableSizeInBytes = stringTable.SizeInBytes();
	header.BinaryBlobSizeInBytes = static_cast<std::uint32_t>(binaryBlob.size());
	header.ShaderPackageKey = ::BuildShaderPackageKey(package.packageId, package.variantId);
	header.SourceIdentityHash = SourceIdentityHasher::Compute(package, compiledStages);
	header.BindingLayoutHash = BuildPassParameterLayoutHash(bindingLayout);
	header.VariantHash = BuildShaderVariantHash(package.variantId);

	const std::filesystem::path packagePath = ::BuildCookedShaderPackagePath(header.ShaderPackageKey);
	std::ofstream output;
	if (!TryOpenBinaryOutput(packagePath, output, outErrorMessage))
	{
		return false;
	}

	if (!BinaryStreamWriter::WriteValue(output, header, outErrorMessage) ||
	    !BinaryStreamWriter::WriteArray(output, binaryRecords, outErrorMessage) ||
	    !BinaryStreamWriter::WriteArray(output, bindingRecords, outErrorMessage) ||
	    !BinaryStreamWriter::WriteArray(output, specializationInputs, outErrorMessage) ||
	    !BinaryStreamWriter::WriteArray(output, stringTable.GetBytes(), outErrorMessage) ||
	    !BinaryStreamWriter::WriteArray(output, binaryBlob, outErrorMessage))
	{
		return false;
	}

	outPackageOutput.packageId = package.packageId;
	outPackageOutput.variantId = package.variantId;
	outPackageOutput.bindingLayoutId = package.bindingLayoutId;
	outPackageOutput.outputPath = packagePath;
	outPackageOutput.packageKey = header.ShaderPackageKey;
	outPackageOutput.sourceIdentityHash = header.SourceIdentityHash;
	outPackageOutput.bindingLayoutHash = header.BindingLayoutHash;
	outPackageOutput.variantHash = header.VariantHash;
	outPackageOutput.declaredStages = header.DeclaredStages;
	outErrorMessage.clear();
	return true;
}
