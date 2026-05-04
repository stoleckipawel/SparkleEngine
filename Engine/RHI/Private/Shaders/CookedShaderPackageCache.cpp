#include "PCH.h"

#include "Shaders/CookedShaderPackageCache.h"

#include "Config/RenderConfig.h"
#include "Core/Public/Files/BinarySpanReader.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "ShaderParameters/PassParameterLayout.h"

#include <array>
#include <format>
#include <vector>

namespace
{
	const char* FormatCookedShaderBinaryFormat(CookedShaderBinaryFormat format) noexcept
	{
		switch (format)
		{
			case CookedShaderBinaryFormat::Dxil:  return "DXIL";
			case CookedShaderBinaryFormat::SpirV: return "SPIR-V";
		}
		return "unknown";
	}

	constexpr std::array<ShaderStage, 6> kKnownShaderStages =
	    {ShaderStage::Vertex, ShaderStage::Pixel, ShaderStage::Geometry, ShaderStage::Hull, ShaderStage::Domain, ShaderStage::Compute};

	bool HasAllStages(ShaderStageMask value, ShaderStageMask flags) noexcept
	{
		return (static_cast<std::uint8_t>(value) & static_cast<std::uint8_t>(flags)) == static_cast<std::uint8_t>(flags);
	}

	ShaderStageMask ToPackageStageMask(ShaderStageVisibility visibility) noexcept
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
}  // namespace

const CookedShaderBinaryRecord* LoadedShaderPackage::FindBinaryRecord(ShaderStage stage, CookedShaderBinaryFormat format) const noexcept
{
	for (const CookedShaderBinaryRecord& binaryRecord : m_binaryRecords)
	{
		if (binaryRecord.Stage == stage && binaryRecord.Format == format)
		{
			return &binaryRecord;
		}
	}

	return nullptr;
}

ShaderBytecode LoadedShaderPackage::GetStageBytecode(ShaderStage stage, CookedShaderBinaryFormat format) const noexcept
{
	const CookedShaderBinaryRecord* binaryRecord = FindBinaryRecord(stage, format);
	return binaryRecord != nullptr ? GetBytecode(*binaryRecord) : ShaderBytecode{};
}

ShaderBytecode LoadedShaderPackage::GetBytecode(const CookedShaderBinaryRecord& record) const noexcept
{
	if (!ContainsBlobRef(record.Bytecode))
	{
		return {};
	}

	const std::uint8_t* bytecodeBegin = m_binaryBlob.data() + record.Bytecode.OffsetInBytes;
	return {bytecodeBegin, record.Bytecode.SizeInBytes};
}

std::string_view LoadedShaderPackage::ResolveString(CookedShaderStringRef ref) const noexcept
{
	if (!ContainsStringRef(ref))
	{
		return {};
	}

	const char* stringBegin = reinterpret_cast<const char*>(m_stringTable.data() + ref.OffsetInBytes);
	return std::string_view(stringBegin, ref.SizeInBytes);
}

bool LoadedShaderPackage::ContainsStringRef(CookedShaderStringRef ref) const noexcept
{
	return ref.OffsetInBytes <= m_stringTable.size() && ref.SizeInBytes <= m_stringTable.size() - ref.OffsetInBytes;
}

bool LoadedShaderPackage::ContainsBlobRef(CookedShaderBlobRef ref) const noexcept
{
	return ref.OffsetInBytes <= m_binaryBlob.size() && ref.SizeInBytes <= m_binaryBlob.size() - ref.OffsetInBytes;
}

void CookedShaderPackageCache::Clear() noexcept
{
	m_packages.clear();
	++m_generation;
}

void CookedShaderPackageCache::ReplaceWith(CookedShaderPackageCache&& replacement) noexcept
{
	m_packages = std::move(replacement.m_packages);
	++m_generation;
}

bool CookedShaderPackageCache::LoadPackage(
    const ShaderPackageDefinition& definition,
    const PassParameterLayout& expectedBindingLayout,
	CookedShaderBinaryFormat requiredBinaryFormat,
    std::string& outErrorMessage,
    const LoadedShaderPackage*& outPackage)
{
	outPackage = nullptr;
	if (!definition.IsValid())
	{
		outErrorMessage = "Shader package definition is invalid.";
		return false;
	}

	const std::uint64_t packageKey = BuildShaderPackageKey(definition.PackageId);
	if (auto it = m_packages.find(packageKey); it != m_packages.end())
	{
		if (!ValidatePackage(*it->second, definition, expectedBindingLayout, requiredBinaryFormat, outErrorMessage))
		{
			return false;
		}

		outPackage = it->second.get();
		outErrorMessage.clear();
		return true;
	}

	auto loadedPackage = std::make_unique<LoadedShaderPackage>();
	const std::filesystem::path packagePath = Paths::CookedShaderPackage(packageKey);
	if (!LoadPackageFromFile(packagePath, *loadedPackage, outErrorMessage))
	{
		return false;
	}

	if (!ValidatePackage(*loadedPackage, definition, expectedBindingLayout, requiredBinaryFormat, outErrorMessage))
	{
		return false;
	}

	LoadedShaderPackage* cachedPackage = loadedPackage.get();
	m_packages.emplace(packageKey, std::move(loadedPackage));
	outPackage = cachedPackage;
	outErrorMessage.clear();
	return true;
}

bool CookedShaderPackageCache::ReloadPackage(
    const ShaderPackageDefinition& definition,
    const PassParameterLayout& expectedBindingLayout,
	CookedShaderBinaryFormat requiredBinaryFormat,
    std::string& outErrorMessage,
    const LoadedShaderPackage*& outPackage)
{
	outPackage = nullptr;
	if (!definition.IsValid())
	{
		outErrorMessage = "Shader package definition is invalid.";
		return false;
	}

	const std::uint64_t packageKey = BuildShaderPackageKey(definition.PackageId);
	auto loadedPackage = std::make_unique<LoadedShaderPackage>();
	const std::filesystem::path packagePath = Paths::CookedShaderPackage(packageKey);
	if (!LoadPackageFromFile(packagePath, *loadedPackage, outErrorMessage))
	{
		return false;
	}

	if (!ValidatePackage(*loadedPackage, definition, expectedBindingLayout, requiredBinaryFormat, outErrorMessage))
	{
		return false;
	}

	LoadedShaderPackage* cachedPackage = loadedPackage.get();
	m_packages[packageKey] = std::move(loadedPackage);
	++m_generation;
	outPackage = cachedPackage;
	outErrorMessage.clear();
	return true;
}

bool CookedShaderPackageCache::LoadPackageFromFile(
    const std::filesystem::path& path,
    LoadedShaderPackage& outPackage,
    std::string& outErrorMessage)
{
	std::vector<std::uint8_t> fileBytes;
	if (!Files::TryReadAllBytes(path, fileBytes, outErrorMessage))
	{
		return false;
	}

	Files::BinarySpanReader reader(fileBytes);
	if (!reader.ReadValue(outPackage.m_header, outErrorMessage))
	{
		return false;
	}

	if (!outPackage.m_header.Matches(kCookedShaderPackageMagic, kCookedShaderPackageVersion))
	{
		if (outPackage.m_header.Magic == kCookedShaderPackageMagic)
		{
			outErrorMessage = std::format(
			    "Cooked shader package '{}' is version {}. Recook to current version {}.",
			    path.string(),
			    outPackage.m_header.Version,
			    kCookedShaderPackageVersion);
			return false;
		}

		outErrorMessage = std::format(
		    "Invalid cooked shader package header in '{}': expected magic {} version {}, got magic {} version {}",
		    path.string(),
		    Formatting::FormatHexUInt32(kCookedShaderPackageMagic),
		    kCookedShaderPackageVersion,
		    Formatting::FormatHexUInt32(outPackage.m_header.Magic),
		    outPackage.m_header.Version);
		return false;
	}

	if (!reader.ReadArray(outPackage.m_header.BinaryRecordCount, outPackage.m_binaryRecords, outErrorMessage) ||
	    !reader.ReadArray(outPackage.m_header.BindingRecordCount, outPackage.m_bindingRecords, outErrorMessage) ||
	    !reader.ReadArray(outPackage.m_header.SpecializationInputCount, outPackage.m_specializationInputs, outErrorMessage) ||
	    !reader.ReadArray(outPackage.m_header.ReflectionRecordCount, outPackage.m_reflectionRecords, outErrorMessage) ||
	    !reader.ReadArray(outPackage.m_header.ResourceBindingRecordCount, outPackage.m_resourceBindings, outErrorMessage) ||
	    !reader.ReadArray(outPackage.m_header.ConstantBufferRecordCount, outPackage.m_constantBuffers, outErrorMessage) ||
	    !reader.ReadArray(outPackage.m_header.ConstantBufferMemberRecordCount, outPackage.m_constantBufferMembers, outErrorMessage) ||
	    !reader.ReadArray(outPackage.m_header.InputElementRecordCount, outPackage.m_inputElements, outErrorMessage) ||
	    !reader.ReadArray(outPackage.m_header.PushConstantRangeRecordCount, outPackage.m_pushConstantRanges, outErrorMessage) ||
	    !reader.ReadArray(outPackage.m_header.SpecializationConstantRecordCount, outPackage.m_specializationConstants, outErrorMessage) ||
	    !reader.ReadArray(outPackage.m_header.RayTracingExportRecordCount, outPackage.m_rayTracingExports, outErrorMessage) ||
	    !reader.ReadArray(outPackage.m_header.RayTracingHitGroupRecordCount, outPackage.m_rayTracingHitGroups, outErrorMessage) ||
	    !reader.ReadArray(outPackage.m_header.RayTracingLocalParameterRecordCount, outPackage.m_rayTracingLocalParameters, outErrorMessage) ||
	    !reader.ReadArray(outPackage.m_header.StringTableSizeInBytes, outPackage.m_stringTable, outErrorMessage) ||
	    !reader.ReadArray(outPackage.m_header.BinaryBlobSizeInBytes, outPackage.m_binaryBlob, outErrorMessage))
	{
		return false;
	}

	if (reader.GetRemainingByteCount() != 0)
	{
		outErrorMessage = std::format("Cooked shader package '{}' contains unexpected trailing bytes", path.string());
		return false;
	}

	outPackage.m_isValid = true;
	outErrorMessage.clear();
	return true;
}

bool CookedShaderPackageCache::ValidatePackage(
    const LoadedShaderPackage& package,
    const ShaderPackageDefinition& definition,
    const PassParameterLayout& expectedBindingLayout,
	CookedShaderBinaryFormat requiredBinaryFormat,
    std::string& outErrorMessage)
{
	if (!package.IsValid())
	{
		outErrorMessage = "Cooked shader package payload is invalid.";
		return false;
	}

	if (package.GetHeader().PackageKind == CookedShaderPackageKind::RayTracingLibrary)
	{
		outErrorMessage = std::format(
		    "Cooked shader package '{}' is a ray tracing library package; runtime RT state object execution is not implemented yet.",
		    definition.PackageId);
		return false;
	}

	const std::uint64_t expectedPackageKey = BuildShaderPackageKey(definition.PackageId);
	if (package.GetHeader().ShaderPackageKey != expectedPackageKey)
	{
		outErrorMessage = std::format(
		    "Cooked shader package '{}' failed compatibility check: field=ShaderPackageKey expected={} actual={}",
		    definition.PackageId,
		    Formatting::FormatHexUInt64(expectedPackageKey),
		    Formatting::FormatHexUInt64(package.GetHeader().ShaderPackageKey));
		return false;
	}

	if (package.GetHeader().SourceIdentityHash == 0)
	{
		outErrorMessage = std::format(
		    "Cooked shader package '{}' failed compatibility check: field=SourceIdentityHash actual=0",
		    definition.PackageId);
		return false;
	}

	const std::uint64_t expectedBindingLayoutHash = BuildPassParameterLayoutHash(expectedBindingLayout);
	if (package.GetHeader().BindingLayoutHash != expectedBindingLayoutHash)
	{
		outErrorMessage = std::format(
		    "Cooked shader package '{}' failed compatibility check: field=BindingLayoutHash bindingLayout='{}' expected={} actual={}",
		    definition.PackageId,
		    definition.BindingLayoutId != nullptr ? definition.BindingLayoutId : expectedBindingLayout.GetDebugName().c_str(),
		    Formatting::FormatHexUInt64(expectedBindingLayoutHash),
		    Formatting::FormatHexUInt64(package.GetHeader().BindingLayoutHash));
		return false;
	}

	if (package.GetHeader().ShaderModelMajor != static_cast<std::uint16_t>(RenderConfig::ShaderModelMajor) ||
	    package.GetHeader().ShaderModelMinor != static_cast<std::uint16_t>(RenderConfig::ShaderModelMinor))
	{
		outErrorMessage = std::format(
		    "Cooked shader package '{}' failed compatibility check: field=ShaderModel expected={}.{} actual={}.{}",
		    definition.PackageId,
		    RenderConfig::ShaderModelMajor,
		    RenderConfig::ShaderModelMinor,
		    package.GetHeader().ShaderModelMajor,
		    package.GetHeader().ShaderModelMinor);
		return false;
	}

	if (!HasAllStages(package.GetHeader().DeclaredStages, definition.ExpectedStages))
	{
		outErrorMessage = std::format(
		    "Cooked shader package '{}' failed compatibility check: field=DeclaredStages expected='{}' actual='{}'",
		    definition.PackageId,
		    FormatShaderStageMask(definition.ExpectedStages),
		    FormatShaderStageMask(package.GetHeader().DeclaredStages));
		return false;
	}

	const std::vector<PassParameterDesc>& expectedParameters = expectedBindingLayout.GetParameters();
	if (package.GetBindingRecords().size() != expectedParameters.size())
	{
		outErrorMessage = std::format(
		    "Cooked shader package '{}' declares {} binding records but runtime layout '{}' expects {}",
		    definition.PackageId,
		    package.GetBindingRecords().size(),
		    definition.BindingLayoutId != nullptr ? definition.BindingLayoutId : expectedBindingLayout.GetDebugName().c_str(),
		    expectedParameters.size());
		return false;
	}

	for (std::size_t parameterIndex = 0; parameterIndex < expectedParameters.size(); ++parameterIndex)
	{
		const PassParameterDesc& expectedParameter = expectedParameters[parameterIndex];
		const CookedShaderBindingRecord& bindingRecord = package.GetBindingRecords()[parameterIndex];
		const std::string_view bindingName = package.ResolveString(bindingRecord.Name);
		if (bindingName.empty())
		{
			outErrorMessage = std::format(
			    "Cooked shader package '{}' has an invalid binding name string for binding index {}",
			    definition.PackageId,
			    parameterIndex);
			return false;
		}

		if (bindingRecord.LogicalBindingIndex != parameterIndex || bindingName != expectedParameter.Name ||
		    bindingRecord.SemanticKind != expectedParameter.Kind || bindingRecord.ResourceDomain != expectedParameter.ResourceDomain ||
		    bindingRecord.Access != expectedParameter.Access ||
		    bindingRecord.VisibilityMask != ToPackageStageMask(expectedParameter.Visibility) ||
		    bindingRecord.ArrayCount != expectedParameter.ArrayCount ||
		    bindingRecord.ValueSizeInBytes != expectedParameter.ValueSizeInBytes)
		{
			outErrorMessage = std::format(
			    "Cooked shader package '{}' binding record {} does not match runtime layout parameter '{}'",
			    definition.PackageId,
			    parameterIndex,
			    expectedParameter.Name);
			return false;
		}
	}

	std::array<bool, static_cast<std::size_t>(ShaderStage::Count)> hasRequiredBinaryForStage = {};
	for (const CookedShaderBinaryRecord& binaryRecord : package.m_binaryRecords)
	{
		if (binaryRecord.Stage == ShaderStage::Count)
		{
			outErrorMessage = std::format(
			    "Cooked shader package '{}' contains an invalid shader stage record",
			    definition.PackageId);
			return false;
		}

		if (package.ResolveString(binaryRecord.EntryPoint).empty())
		{
			outErrorMessage = std::format(
			    "Cooked shader package '{}' contains an invalid entry point string",
			    definition.PackageId);
			return false;
		}

		if (binaryRecord.DebugArtifact && package.ResolveString(binaryRecord.DebugArtifact).empty())
		{
			outErrorMessage = std::format(
			    "Cooked shader package '{}' contains an invalid debug artifact string",
			    definition.PackageId);
			return false;
		}

		const ShaderBytecode bytecode = package.GetBytecode(binaryRecord);
		if (!bytecode.IsValid())
		{
			outErrorMessage = std::format(
			    "Cooked shader package '{}' contains an invalid bytecode blob for stage {}",
			    definition.PackageId,
			    static_cast<std::uint32_t>(binaryRecord.Stage));
			return false;
		}

		if (Hash::Fnv1a64(bytecode.Data, bytecode.Size) != binaryRecord.BytecodeHash)
		{
			outErrorMessage = std::format(
			    "Cooked shader package '{}' failed bytecode hash validation for stage {}",
			    definition.PackageId,
			    static_cast<std::uint32_t>(binaryRecord.Stage));
			return false;
		}

		if (!HasAllStages(package.GetHeader().DeclaredStages, ToShaderStageMask(binaryRecord.Stage)))
		{
			outErrorMessage = std::format(
			    "Cooked shader package '{}' contains a stage record outside its declared stage mask",
			    definition.PackageId);
			return false;
		}

		if (binaryRecord.Format == requiredBinaryFormat)
		{
			const std::size_t stageIndex = static_cast<std::size_t>(binaryRecord.Stage);
			if (hasRequiredBinaryForStage[stageIndex])
			{
				outErrorMessage = std::format(
				    "Cooked shader package '{}' contains more than one {} binary for stage {}",
				    definition.PackageId,
				    FormatCookedShaderBinaryFormat(requiredBinaryFormat),
				    stageIndex);
				return false;
			}

			hasRequiredBinaryForStage[stageIndex] = true;
		}
	}

	for (const ShaderStage stage : kKnownShaderStages)
	{
		if (!HasAllStages(definition.ExpectedStages, ToShaderStageMask(stage)))
		{
			continue;
		}

		if (!hasRequiredBinaryForStage[static_cast<std::size_t>(stage)])
		{
			outErrorMessage = std::format(
			    "Cooked shader package '{}' is missing the required {} binary for stage {}",
			    definition.PackageId,
			    FormatCookedShaderBinaryFormat(requiredBinaryFormat),
			    static_cast<std::uint32_t>(stage));
			return false;
		}
	}

	outErrorMessage.clear();
	return true;
}