#include "PCH.h"

#include "Cooking/CookedPackageWriter.h"

#include "Constants/ShaderCompilerConstants.h"
#include "Cooking/BindingRecordBuilder.h"
#include "Cooking/ReflectionSerializer.h"
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

	std::vector<ShaderReflection> reflectionsForSerialization;
	reflectionsForSerialization.reserve(compiledStages.size());

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
		        .Format = compiledStage.format,
		        .BytecodeHash = compiledStage.bytecodeHash,
		        .BackendName = ToCookedShaderStringRef(stringTable.Add(compiledStage.backendName)),
		        .BackendVersion = compiledStage.backendVersion});

		declaredStages |= ToShaderStageMask(compiledStage.stage);
		reflectionsForSerialization.push_back(compiledStage.reflection);
	}

	ReflectionSerializer::Output reflectionOutput;
	ReflectionSerializer::Build(reflectionsForSerialization, stringTable, reflectionOutput);

	CookedShaderPackageHeader header{};
	header.DeclaredStages = declaredStages;
	header.ShaderModelMajor = static_cast<std::uint16_t>(RenderConfig::ShaderModelMajor);
	header.ShaderModelMinor = static_cast<std::uint16_t>(RenderConfig::ShaderModelMinor);
	header.BinaryRecordCount = static_cast<std::uint32_t>(binaryRecords.size());
	header.BindingRecordCount = static_cast<std::uint32_t>(bindingRecords.size());
	header.SpecializationInputCount = static_cast<std::uint32_t>(specializationInputs.size());
	header.StringTableSizeInBytes = stringTable.SizeInBytes();
	header.BinaryBlobSizeInBytes = static_cast<std::uint32_t>(binaryBlob.size());
	header.ReflectionRecordCount = static_cast<std::uint32_t>(reflectionOutput.reflectionRecords.size());
	header.ResourceBindingRecordCount = static_cast<std::uint32_t>(reflectionOutput.resourceBindings.size());
	header.ConstantBufferRecordCount = static_cast<std::uint32_t>(reflectionOutput.constantBuffers.size());
	header.ConstantBufferMemberRecordCount = static_cast<std::uint32_t>(reflectionOutput.constantBufferMembers.size());
	header.InputElementRecordCount = static_cast<std::uint32_t>(reflectionOutput.inputElements.size());
	header.PushConstantRangeRecordCount = static_cast<std::uint32_t>(reflectionOutput.pushConstantRanges.size());
	header.SpecializationConstantRecordCount = static_cast<std::uint32_t>(reflectionOutput.specializationConstants.size());
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
	    !BinaryStreamWriter::WriteArray(output, reflectionOutput.reflectionRecords, outErrorMessage) ||
	    !BinaryStreamWriter::WriteArray(output, reflectionOutput.resourceBindings, outErrorMessage) ||
	    !BinaryStreamWriter::WriteArray(output, reflectionOutput.constantBuffers, outErrorMessage) ||
	    !BinaryStreamWriter::WriteArray(output, reflectionOutput.constantBufferMembers, outErrorMessage) ||
	    !BinaryStreamWriter::WriteArray(output, reflectionOutput.inputElements, outErrorMessage) ||
	    !BinaryStreamWriter::WriteArray(output, reflectionOutput.pushConstantRanges, outErrorMessage) ||
	    !BinaryStreamWriter::WriteArray(output, reflectionOutput.specializationConstants, outErrorMessage) ||
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
