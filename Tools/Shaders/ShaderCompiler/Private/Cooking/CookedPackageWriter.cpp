#include "PCH.h"

#include "Cooking/CookedPackageWriter.h"

#include "Constants/ShaderCompilerConstants.h"
#include "Cooking/ReflectionSerializer.h"
#include "Cooking/SourceIdentityHasher.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Files/BinaryStreamWriter.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Strings/StringTableBuilder.h"

#include "RHI/Public/Shaders/CookedShaderPackageContract.h"
#include "RHI/Public/Shaders/CookedShaderPackageIdentity.h"

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
		outBindingRecords.push_back(
		    CookedShaderBindingRecord{
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

static void AddPipelineLayoutCounts(const CookedShaderBindingRecord& bindingRecord, CookedShaderPipelineLayoutRecord& layoutRecord) noexcept
{
	switch (bindingRecord.SemanticKind)
	{
		case ShaderParameterSemanticKind::UniformData:
			++layoutRecord.ConstantBufferCount;
			break;
		case ShaderParameterSemanticKind::ReadTexture:
		case ShaderParameterSemanticKind::ReadBuffer:
			layoutRecord.DescriptorBindingCount += std::max(1u, bindingRecord.ArrayCount);
			++layoutRecord.ReadOnlyResourceCount;
			break;
		case ShaderParameterSemanticKind::RWTexture:
		case ShaderParameterSemanticKind::RWBuffer:
			layoutRecord.DescriptorBindingCount += std::max(1u, bindingRecord.ArrayCount);
			++layoutRecord.ReadWriteResourceCount;
			break;
		case ShaderParameterSemanticKind::SamplerSet:
			layoutRecord.DescriptorBindingCount += std::max(1u, bindingRecord.ArrayCount);
			++layoutRecord.SamplerCount;
			break;
		case ShaderParameterSemanticKind::AccelerationStructure:
			layoutRecord.DescriptorBindingCount += std::max(1u, bindingRecord.ArrayCount);
			++layoutRecord.AccelerationStructureCount;
			break;
		case ShaderParameterSemanticKind::RenderTarget:
		case ShaderParameterSemanticKind::DepthTarget:
		default:
			break;
	}
}

static std::uint32_t EstimateDescriptorSetCount(const std::vector<CookedShaderBindingRecord>& bindingRecords) noexcept
{
	return bindingRecords.empty() ? 0u : 1u;
}

static void BuildPipelineLayoutRecords(
    std::span<const CookedStageBuild> compiledStages,
    const std::vector<CookedShaderBindingRecord>& bindingRecords,
    std::uint64_t bindingLayoutHash,
    Strings::StringTableBuilder& stringTable,
    std::vector<CookedShaderPipelineLayoutRecord>& outRecords)
{
	outRecords.clear();
	std::vector<std::string> codegenTargets;
	for (const CookedStageBuild& compiledStage : compiledStages)
	{
		if (compiledStage.codegenTarget.empty() || std::ranges::find(codegenTargets, compiledStage.codegenTarget) != codegenTargets.end())
		{
			continue;
		}
		codegenTargets.push_back(compiledStage.codegenTarget);
	}
	std::ranges::sort(codegenTargets);

	outRecords.reserve(codegenTargets.size());
	for (const std::string& codegenTarget : codegenTargets)
	{
		CookedShaderPipelineLayoutRecord layoutRecord{};
		layoutRecord.CodegenTarget = ToCookedShaderStringRef(stringTable.Add(codegenTarget));
		layoutRecord.BindingLayoutHash = bindingLayoutHash;
		layoutRecord.BindingRecordOffset = 0;
		layoutRecord.BindingRecordCount = static_cast<std::uint32_t>(bindingRecords.size());
		layoutRecord.DescriptorSetCount = EstimateDescriptorSetCount(bindingRecords);

		for (const CookedShaderBindingRecord& bindingRecord : bindingRecords)
		{
			if (bindingRecord.SemanticKind == ShaderParameterSemanticKind::UniformData && bindingRecord.ValueSizeInBytes > 0)
			{
				++layoutRecord.PushConstantRangeCount;
				layoutRecord.PushConstantSizeInBytes += bindingRecord.ValueSizeInBytes;
			}
			AddPipelineLayoutCounts(bindingRecord, layoutRecord);
		}

		outRecords.push_back(layoutRecord);
	}
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
		if (compiledStage.stage == stage.stage && compiledStage.sourcePath == stage.sourcePath &&
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
		outRecords.push_back(
		    CookedShaderRayTracingExportRecord{
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
    std::string_view exportLookupName,
    CookedShaderRayTracingExportKind expectedKind) noexcept
{
	if (exportLookupName.empty())
	{
		return UINT32_MAX;
	}

	for (std::size_t exportIndex = 0; exportIndex < exports.size(); ++exportIndex)
	{
		const ShaderCookRayTracingExportDesc& rtExport = exports[exportIndex];
		if (rtExport.exportLookupName == exportLookupName && rtExport.kind == expectedKind)
		{
			return static_cast<std::uint32_t>(exportIndex);
		}
	}

	return UINT32_MAX;
}

static std::vector<CookedShaderRayTracingHitGroupRecord> BuildRayTracingHitGroupRecords(
    const ShaderCookPackageDesc& package,
    Strings::StringTableBuilder& stringTable)
{
	std::vector<CookedShaderRayTracingHitGroupRecord> records;
	records.reserve(package.rayTracingHitGroups.size());
	for (const ShaderCookRayTracingHitGroupDesc& hitGroup : package.rayTracingHitGroups)
	{
		const std::uint32_t closestHitExportIndex = FindRayTracingExportIndex(
		    package.rayTracingExports,
		    hitGroup.closestHitExportName,
		    CookedShaderRayTracingExportKind::ClosestHit);
		if (closestHitExportIndex == UINT32_MAX)
		{
			throw Diagnostics::Error(std::format(
			    "Ray tracing hit group '{}' references missing closest-hit export '{}' in package '{}'",
			    hitGroup.name,
			    hitGroup.closestHitExportName,
			    package.packageId));
		}

		const std::uint32_t anyHitExportIndex =
		    FindRayTracingExportIndex(package.rayTracingExports, hitGroup.anyHitExportName, CookedShaderRayTracingExportKind::AnyHit);
		if (!hitGroup.anyHitExportName.empty() && anyHitExportIndex == UINT32_MAX)
		{
			throw Diagnostics::Error(std::format(
			    "Ray tracing hit group '{}' references missing any-hit export '{}' in package '{}'",
			    hitGroup.name,
			    hitGroup.anyHitExportName,
			    package.packageId));
		}

		const std::uint32_t intersectionExportIndex = FindRayTracingExportIndex(
		    package.rayTracingExports,
		    hitGroup.intersectionExportName,
		    CookedShaderRayTracingExportKind::Intersection);
		if (!hitGroup.intersectionExportName.empty() && intersectionExportIndex == UINT32_MAX)
		{
			throw Diagnostics::Error(std::format(
			    "Ray tracing hit group '{}' references missing intersection export '{}' in package '{}'",
			    hitGroup.name,
			    hitGroup.intersectionExportName,
			    package.packageId));
		}

		records.push_back(
		    CookedShaderRayTracingHitGroupRecord{
		        .HitGroupName = ToCookedShaderStringRef(stringTable.Add(hitGroup.name)),
		        .Type = intersectionExportIndex != UINT32_MAX ? CookedShaderRayTracingHitGroupType::ProceduralPrimitive
		                                                      : CookedShaderRayTracingHitGroupType::Triangles,
		        .ClosestHitExportIndex = closestHitExportIndex,
		        .AnyHitExportIndex = anyHitExportIndex,
		        .IntersectionExportIndex = intersectionExportIndex,
		        .HitGroupHash = Hash::Fnv1a64(hitGroup.name)});
	}

	return records;
}

CookedShaderPackageOutput CookedPackageWriter::Write(
    const ShaderCookPackageDesc& package,
    std::span<const CookedStageBuild> compiledStages,
    const std::filesystem::path& storagePath,
    const std::filesystem::path& publishedPath)
{
	Strings::StringTableBuilder stringTable;
	std::vector<CookedShaderBinaryRecord> binaryRecords;
	std::vector<CookedShaderBindingRecord> bindingRecords;
	std::vector<CookedShaderPipelineLayoutRecord> pipelineLayoutRecords;
	std::vector<CookedShaderSpecializationInputRecord> specializationInputs;
	std::vector<CookedShaderRayTracingExportRecord> rayTracingExportRecords;
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
		        .ShaderBlobId = compiledStage.shaderBlobId,
		        .EntryPoint = ToCookedShaderStringRef(stringTable.Add(compiledStage.entryPoint)),
		        .ExportName = {},
		        .DebugArtifact = ToCookedShaderStringRef(stringTable.Add(compiledStage.debugArtifact)),
		        .Bytecode = CookedShaderBlobRef{blobOffset, static_cast<std::uint32_t>(compiledStage.bytecode.size())},
		        .Stage = compiledStage.stage,
		        .Format = compiledStage.format,
		        .BytecodeHash = compiledStage.bytecodeHash,
		        .BackendName = ToCookedShaderStringRef(stringTable.Add(compiledStage.backendName)),
		        .CodegenTarget = ToCookedShaderStringRef(stringTable.Add(compiledStage.codegenTarget)),
		        .BackendVersion = compiledStage.backendVersion});

		declaredStages |= ToShaderStageMask(compiledStage.stage);
		reflectionsForSerialization.push_back(compiledStage.reflection);
	}

	const std::uint64_t bindingLayoutHash = BuildPassParameterLayoutHash(package.bindingLayout);
	BuildPipelineLayoutRecords(compiledStages, bindingRecords, bindingLayoutHash, stringTable, pipelineLayoutRecords);

	ReflectionSerializer::Output reflectionOutput;
	ReflectionSerializer::Build(reflectionsForSerialization, stringTable, reflectionOutput);
	BuildRayTracingExportRecords(package, compiledStages, stringTable, rayTracingExportRecords);
	const std::vector<CookedShaderRayTracingHitGroupRecord> rayTracingHitGroupRecords =
	    BuildRayTracingHitGroupRecords(package, stringTable);

	CookedShaderPackageHeader header{};
	header.DeclaredStages = declaredStages;
	header.PackageKind = package.packageKind;
	header.PackageFeatures = package.packageFeatures;
	if (LayoutUsesAccelerationStructure(package.bindingLayout))
	{
		header.PackageFeatures |= CookedShaderPackageFeatureFlags::UsesAccelerationStructure;
	}
	header.ShaderModelMajor = CookedShaderPackageContract::ShaderModelMajor;
	header.ShaderModelMinor = CookedShaderPackageContract::ShaderModelMinor;
	header.BinaryRecordCount = static_cast<std::uint32_t>(binaryRecords.size());
	header.BindingRecordCount = static_cast<std::uint32_t>(bindingRecords.size());
	header.PipelineLayoutRecordCount = static_cast<std::uint32_t>(pipelineLayoutRecords.size());
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
	header.ShaderPackageKey = ::BuildShaderPackageKey(package.packageId);
	header.SourceIdentityHash = SourceIdentityHasher::Compute(package, compiledStages);
	header.BindingLayoutHash = bindingLayoutHash;

	const std::filesystem::path tempPackagePath = Files::BuildTemporaryPath(storagePath);
	std::ofstream output;
	std::string fileError;
	if (!Files::TryOpenBinaryOutput(tempPackagePath, output, fileError))
	{
		throw Diagnostics::Error(std::move(fileError));
	}

	if (!Files::BinaryStreamWriter::WriteValue(output, header, fileError) ||
	    !Files::BinaryStreamWriter::WriteArray(output, binaryRecords, fileError) ||
	    !Files::BinaryStreamWriter::WriteArray(output, bindingRecords, fileError) ||
	    !Files::BinaryStreamWriter::WriteArray(output, pipelineLayoutRecords, fileError) ||
	    !Files::BinaryStreamWriter::WriteArray(output, specializationInputs, fileError) ||
	    !Files::BinaryStreamWriter::WriteArray(output, reflectionOutput.reflectionRecords, fileError) ||
	    !Files::BinaryStreamWriter::WriteArray(output, reflectionOutput.resourceBindings, fileError) ||
	    !Files::BinaryStreamWriter::WriteArray(output, reflectionOutput.constantBuffers, fileError) ||
	    !Files::BinaryStreamWriter::WriteArray(output, reflectionOutput.constantBufferMembers, fileError) ||
	    !Files::BinaryStreamWriter::WriteArray(output, reflectionOutput.inputElements, fileError) ||
	    !Files::BinaryStreamWriter::WriteArray(output, reflectionOutput.pushConstantRanges, fileError) ||
	    !Files::BinaryStreamWriter::WriteArray(output, reflectionOutput.specializationConstants, fileError) ||
	    !Files::BinaryStreamWriter::WriteArray(output, rayTracingExportRecords, fileError) ||
	    !Files::BinaryStreamWriter::WriteArray(output, rayTracingHitGroupRecords, fileError) ||
	    !Files::BinaryStreamWriter::WriteArray(output, rayTracingLocalParameterRecords, fileError) ||
	    !Files::BinaryStreamWriter::WriteArray(output, stringTable.GetBytes(), fileError) ||
	    !Files::BinaryStreamWriter::WriteArray(output, binaryBlob, fileError))
	{
		output.close();
		Files::CleanupTemporaryFile(tempPackagePath);
		throw Diagnostics::Error(std::move(fileError));
	}

	if (!Files::TryCloseOutput(output, tempPackagePath, fileError))
	{
		Files::CleanupTemporaryFile(tempPackagePath);
		throw Diagnostics::Error(std::move(fileError));
	}

	if (!Files::TryFinalizeTemporaryFile(tempPackagePath, storagePath, fileError))
	{
		Files::CleanupTemporaryFile(tempPackagePath);
		throw Diagnostics::Error(std::move(fileError));
	}

	return CookedShaderPackageOutput{
	    .packageId = package.packageId,
	    .bindingLayoutId = package.bindingLayoutId,
	    .outputPath = publishedPath,
	    .packageKey = header.ShaderPackageKey,
	    .sourceIdentityHash = header.SourceIdentityHash,
	    .bindingLayoutHash = header.BindingLayoutHash,
	    .declaredStages = header.DeclaredStages};
}
