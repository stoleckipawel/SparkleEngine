#include "PCH.h"

#include "Cooking/CookedPackageWriter.h"

#include "Constants/ShaderCompilerConstants.h"
#include "Cooking/ReflectionSerializer.h"
#include "Cooking/SourceIdentityHasher.h"
#include "Core/Public/Files/BinaryStreamWriter.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Strings/StringTableBuilder.h"

#include "RHI/Public/Config/RenderConfig.h"
#include "RHI/Public/Shaders/CookedShaderPackageUtils.h"

#include <fstream>

static CookedShaderStringRef ToCookedShaderStringRef(const Strings::StringTableEntry& entry)
{
	return CookedShaderStringRef{entry.OffsetInBytes, entry.SizeInBytes};
}

static ShaderStageMask ToCookedShaderStageMask(ShaderStageVisibility visibility) noexcept
{
	switch (visibility)
	{
		case ShaderStageVisibility::Vertex:
			return ShaderStageMask::Vertex;
		case ShaderStageVisibility::Pixel:
			return ShaderStageMask::Pixel;
		case ShaderStageVisibility::Compute:
			return ShaderStageMask::Compute;
		case ShaderStageVisibility::AllGraphics:
			return ShaderStageMask::Vertex | ShaderStageMask::Pixel;
		case ShaderStageVisibility::All:
			return ShaderStageMask::Vertex | ShaderStageMask::Pixel | ShaderStageMask::Compute;
		case ShaderStageVisibility::None:
		default:
			return ShaderStageMask::None;
	}
}

static void BuildBindingRecords(
	const PassParameterLayout& layout,
	Strings::StringTableBuilder& stringTable,
	std::vector<CookedShaderBindingRecord>& outBindingRecords)
{
	const std::vector<PassParameterDesc>& parameters = layout.GetParameters();
	outBindingRecords.clear();
	outBindingRecords.reserve(parameters.size());

	for (std::size_t parameterIndex = 0; parameterIndex < parameters.size(); ++parameterIndex)
	{
		const PassParameterDesc& parameter = parameters[parameterIndex];
		const Strings::StringTableEntry nameEntry = stringTable.Add(parameter.Name);
		outBindingRecords.push_back(CookedShaderBindingRecord{
		    .Name = ToCookedShaderStringRef(nameEntry),
		    .SemanticKind = parameter.Kind,
		    .ResourceDomain = parameter.ResourceDomain,
		    .Access = parameter.Access,
		    .VisibilityMask = ToCookedShaderStageMask(parameter.Visibility),
		    .LogicalBindingIndex = static_cast<std::uint32_t>(parameterIndex),
		    .ArrayCount = parameter.ArrayCount,
		    .ValueSizeInBytes = parameter.ValueSizeInBytes});
	}
}

static bool LayoutUsesAccelerationStructure(const PassParameterLayout& layout) noexcept
{
	for (const PassParameterDesc& parameter : layout.GetParameters())
	{
		if (parameter.Kind == ShaderParameterSemanticKind::AccelerationStructure)
		{
			return true;
		}
	}
	return false;
}

static std::uint32_t FindBinaryRecordIndexForStage(
    const ShaderCookPackageDesc& package,
    std::span<const CookedStageBuild> compiledStages,
    const ShaderCookRayTracingExportDesc& rtExport) noexcept
{
	if (rtExport.stageIndex >= package.stages.size())
	{
		return UINT32_MAX;
	}

	const ShaderCookStageDesc& stage = package.stages[rtExport.stageIndex];
	for (std::size_t binaryIndex = 0; binaryIndex < compiledStages.size(); ++binaryIndex)
	{
		const CookedStageBuild& compiledStage = compiledStages[binaryIndex];
		if (compiledStage.stage == stage.stage && compiledStage.sourcePath.ends_with(stage.sourcePath.generic_string()) &&
		    compiledStage.entryPoint == stage.entryPoint)
		{
			return static_cast<std::uint32_t>(binaryIndex);
		}
	}

	return UINT32_MAX;
}

static void BuildRayTracingExportRecords(
    const ShaderCookPackageDesc& package,
    std::span<const CookedStageBuild> compiledStages,
    Strings::StringTableBuilder& stringTable,
    std::vector<CookedShaderRayTracingExportRecord>& outRecords)
{
	outRecords.clear();
	outRecords.reserve(package.rayTracingExports.size());

	for (const ShaderCookRayTracingExportDesc& rtExport : package.rayTracingExports)
	{
		const std::string& exportName = rtExport.exportName.empty() ? rtExport.entryPoint : rtExport.exportName;
		outRecords.push_back(CookedShaderRayTracingExportRecord{
		    .ExportName = ToCookedShaderStringRef(stringTable.Add(exportName)),
		    .EntryPoint = ToCookedShaderStringRef(stringTable.Add(rtExport.entryPoint)),
		    .BinaryRecordIndex = FindBinaryRecordIndexForStage(package, compiledStages, rtExport),
		    .Kind = rtExport.kind,
		    .Flags = 0,
		    .ExportHash = Hash::Fnv1a64(exportName)});
	}
}

static std::uint32_t FindRayTracingExportIndex(
    std::span<const ShaderCookRayTracingExportDesc> exports,
    std::string_view shaderName,
    CookedShaderRayTracingExportKind expectedKind) noexcept
{
	if (shaderName.empty())
	{
		return UINT32_MAX;
	}

	for (std::size_t exportIndex = 0; exportIndex < exports.size(); ++exportIndex)
	{
		const ShaderCookRayTracingExportDesc& rtExport = exports[exportIndex];
		if (rtExport.shaderName == shaderName && rtExport.kind == expectedKind)
		{
			return static_cast<std::uint32_t>(exportIndex);
		}
	}

	return UINT32_MAX;
}

static bool BuildRayTracingHitGroupRecords(
    const ShaderCookPackageDesc& package,
    Strings::StringTableBuilder& stringTable,
	std::vector<CookedShaderRayTracingHitGroupRecord>& outRecords,
	std::string& outErrorMessage)
{
	outRecords.clear();
	outRecords.reserve(package.rayTracingHitGroups.size());
	for (const ShaderCookRayTracingHitGroupDesc& hitGroup : package.rayTracingHitGroups)
	{
		const std::uint32_t closestHitExportIndex = FindRayTracingExportIndex(
		    package.rayTracingExports,
		    hitGroup.closestHitShaderName,
		    CookedShaderRayTracingExportKind::ClosestHit);
		if (closestHitExportIndex == UINT32_MAX)
		{
			outErrorMessage = std::format(
			    "Ray tracing hit group '{}' references missing closest-hit shader '{}' in package '{}'",
			    hitGroup.name,
			    hitGroup.closestHitShaderName,
			    package.packageId);
			return false;
		}

		const std::uint32_t anyHitExportIndex = FindRayTracingExportIndex(
		    package.rayTracingExports,
		    hitGroup.anyHitShaderName,
		    CookedShaderRayTracingExportKind::AnyHit);
		if (!hitGroup.anyHitShaderName.empty() && anyHitExportIndex == UINT32_MAX)
		{
			outErrorMessage = std::format(
			    "Ray tracing hit group '{}' references missing any-hit shader '{}' in package '{}'",
			    hitGroup.name,
			    hitGroup.anyHitShaderName,
			    package.packageId);
			return false;
		}

		const std::uint32_t intersectionExportIndex = FindRayTracingExportIndex(
		    package.rayTracingExports,
		    hitGroup.intersectionShaderName,
		    CookedShaderRayTracingExportKind::Intersection);
		if (!hitGroup.intersectionShaderName.empty() && intersectionExportIndex == UINT32_MAX)
		{
			outErrorMessage = std::format(
			    "Ray tracing hit group '{}' references missing intersection shader '{}' in package '{}'",
			    hitGroup.name,
			    hitGroup.intersectionShaderName,
			    package.packageId);
			return false;
		}

		outRecords.push_back(CookedShaderRayTracingHitGroupRecord{
		    .HitGroupName = ToCookedShaderStringRef(stringTable.Add(hitGroup.name)),
		    .Type = intersectionExportIndex != UINT32_MAX ? CookedShaderRayTracingHitGroupType::ProceduralPrimitive
		                                                    : CookedShaderRayTracingHitGroupType::Triangles,
		    .ClosestHitExportIndex = closestHitExportIndex,
		    .AnyHitExportIndex = anyHitExportIndex,
		    .IntersectionExportIndex = intersectionExportIndex,
		    .HitGroupHash = Hash::Fnv1a64(hitGroup.name)});
	}

	outErrorMessage.clear();
	return true;
}

bool CookedPackageWriter::Write(
	const ShaderCookPackageDesc& package,
	std::span<const CookedStageBuild> compiledStages,
	CookedShaderPackageOutput& outPackageOutput,
	std::string& outErrorMessage)
{
	using Files::BinaryStreamWriter;
	using Files::BuildTemporaryPath;
	using Files::TryCloseOutput;
	using Files::TryFinalizeTemporaryFile;
	using Files::TryOpenBinaryOutput;

	Strings::StringTableBuilder stringTable;
	std::vector<CookedShaderBinaryRecord> binaryRecords;
	std::vector<CookedShaderBindingRecord> bindingRecords;
	std::vector<CookedShaderSpecializationInputRecord> specializationInputs;
	std::vector<CookedShaderRayTracingExportRecord> rayTracingExportRecords;
	std::vector<CookedShaderRayTracingHitGroupRecord> rayTracingHitGroupRecords;
	std::vector<CookedShaderRayTracingLocalParameterRecord> rayTracingLocalParameterRecords;
	std::vector<std::uint8_t> binaryBlob;

	BuildBindingRecords(package.bindingLayout, stringTable, bindingRecords);

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
	BuildRayTracingExportRecords(package, compiledStages, stringTable, rayTracingExportRecords);
	if (!BuildRayTracingHitGroupRecords(package, stringTable, rayTracingHitGroupRecords, outErrorMessage))
	{
		return false;
	}

	CookedShaderPackageHeader header{};
	header.DeclaredStages = declaredStages;
	header.PackageKind = package.packageKind;
	header.PackageFeatures = package.packageFeatures;
	if (LayoutUsesAccelerationStructure(package.bindingLayout))
	{
		header.PackageFeatures |= CookedShaderPackageFeatureFlags::UsesAccelerationStructure;
	}
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
	header.RayTracingExportRecordCount = static_cast<std::uint32_t>(rayTracingExportRecords.size());
	header.RayTracingHitGroupRecordCount = static_cast<std::uint32_t>(rayTracingHitGroupRecords.size());
	header.RayTracingLocalParameterRecordCount = static_cast<std::uint32_t>(rayTracingLocalParameterRecords.size());
	header.RayTracingPayloadSizeInBytes = package.rayTracingPayloadSizeInBytes;
	header.RayTracingAttributeSizeInBytes = package.rayTracingAttributeSizeInBytes;
	header.RayTracingMaxRecursionDepth = package.rayTracingMaxRecursionDepth;
	header.ShaderPackageKey = ::BuildShaderPackageKey(package.packageId, package.variantId);
	header.SourceIdentityHash = SourceIdentityHasher::Compute(package, compiledStages);
	header.BindingLayoutHash = BuildPassParameterLayoutHash(package.bindingLayout);
	header.VariantHash = BuildShaderVariantHash(package.variantId);

	const std::filesystem::path packagePath = Paths::CookedShaderPackage(header.ShaderPackageKey);
	const std::filesystem::path tempPackagePath = BuildTemporaryPath(packagePath);
	std::ofstream output;
	if (!TryOpenBinaryOutput(tempPackagePath, output, outErrorMessage))
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
	    !BinaryStreamWriter::WriteArray(output, rayTracingExportRecords, outErrorMessage) ||
	    !BinaryStreamWriter::WriteArray(output, rayTracingHitGroupRecords, outErrorMessage) ||
	    !BinaryStreamWriter::WriteArray(output, rayTracingLocalParameterRecords, outErrorMessage) ||
	    !BinaryStreamWriter::WriteArray(output, stringTable.GetBytes(), outErrorMessage) ||
	    !BinaryStreamWriter::WriteArray(output, binaryBlob, outErrorMessage))
	{
		return false;
	}

	if (!TryCloseOutput(output, tempPackagePath, outErrorMessage))
	{
		return false;
	}

	if (!TryFinalizeTemporaryFile(tempPackagePath, packagePath, outErrorMessage))
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
