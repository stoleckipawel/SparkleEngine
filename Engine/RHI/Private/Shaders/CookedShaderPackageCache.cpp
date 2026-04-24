#include "PCH.h"

#include "Shaders/CookedShaderPackageCache.h"

#include "Config/RenderConfig.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "ShaderParameters/PassParameterLayout.h"

#include <array>
#include <cstring>
#include <format>
#include <limits>
#include <span>
#include <vector>

namespace
{
	std::string FormatStageMask(ShaderStageMask mask)
	{
		if (mask == ShaderStageMask::None)
		{
			return "None";
		}

		struct StageLabel
		{
			ShaderStageMask Mask;
			const char* Label;
		};

		static constexpr std::array<StageLabel, 6> kStageLabels = {{
		    {ShaderStageMask::Vertex, "Vertex"},
		    {ShaderStageMask::Pixel, "Pixel"},
		    {ShaderStageMask::Geometry, "Geometry"},
		    {ShaderStageMask::Hull, "Hull"},
		    {ShaderStageMask::Domain, "Domain"},
		    {ShaderStageMask::Compute, "Compute"},
		}};

		std::string result;
		for (const StageLabel& stageLabel : kStageLabels)
		{
			if (!HasAnyShaderStageMask(mask, stageLabel.Mask))
			{
				continue;
			}

			if (!result.empty())
			{
				result += '|';
			}

			result += stageLabel.Label;
		}

		return result.empty() ? "None" : result;
	}

	class ShaderPackageByteReader final
	{
	  public:
		explicit ShaderPackageByteReader(std::span<const std::uint8_t> bytes) noexcept : m_bytes(bytes) {}

		template <typename T> bool Read(T& outValue, std::string& outErrorMessage)
		{
			static_assert(std::is_trivially_copyable_v<T>, "ShaderPackageByteReader::Read requires trivially copyable types.");

			if (!CanRead(sizeof(T)))
			{
				outErrorMessage = "Unexpected end of cooked shader package data";
				return false;
			}

			std::memcpy(&outValue, m_bytes.data() + m_offset, sizeof(T));
			m_offset += sizeof(T);
			return true;
		}

		template <typename T> bool ReadArray(std::size_t elementCount, std::vector<T>& outValues, std::string& outErrorMessage)
		{
			static_assert(std::is_trivially_copyable_v<T>, "ShaderPackageByteReader::ReadArray requires trivially copyable element types.");

			if constexpr (sizeof(T) > 0)
			{
				if (elementCount > (std::numeric_limits<std::size_t>::max)() / sizeof(T))
				{
					outErrorMessage = "Cooked shader package array size overflow";
					return false;
				}
			}

			const std::size_t byteCount = sizeof(T) * elementCount;
			if (!CanRead(byteCount))
			{
				outErrorMessage = "Unexpected end of cooked shader package array data";
				return false;
			}

			outValues.resize(elementCount);
			if (byteCount > 0)
			{
				std::memcpy(outValues.data(), m_bytes.data() + m_offset, byteCount);
				m_offset += byteCount;
			}

			return true;
		}

		std::size_t GetRemainingByteCount() const noexcept { return m_bytes.size() - m_offset; }

	  private:
		bool CanRead(std::size_t byteCount) const noexcept { return m_offset + byteCount <= m_bytes.size(); }

		std::span<const std::uint8_t> m_bytes;
		std::size_t m_offset = 0;
	};

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

bool CookedShaderPackageCache::LoadPackage(
    const ShaderPackageDefinition& definition,
    const PassParameterLayout& expectedBindingLayout,
    std::string& outErrorMessage,
    const LoadedShaderPackage*& outPackage)
{
	outPackage = nullptr;
	if (!definition.IsValid())
	{
		outErrorMessage = "Shader package definition is invalid.";
		return false;
	}

	const std::uint64_t packageKey = BuildShaderPackageKey(definition.PackageId, definition.VariantId);
	if (auto it = m_packages.find(packageKey); it != m_packages.end())
	{
		if (!ValidatePackage(*it->second, definition, expectedBindingLayout, outErrorMessage))
		{
			return false;
		}

		outPackage = it->second.get();
		outErrorMessage.clear();
		return true;
	}

	auto loadedPackage = std::make_unique<LoadedShaderPackage>();
	const std::filesystem::path packagePath = BuildCookedShaderPackagePath(packageKey);
	if (!LoadPackageFromFile(packagePath, *loadedPackage, outErrorMessage))
	{
		return false;
	}

	if (!ValidatePackage(*loadedPackage, definition, expectedBindingLayout, outErrorMessage))
	{
		return false;
	}

	LoadedShaderPackage* cachedPackage = loadedPackage.get();
	m_packages.emplace(packageKey, std::move(loadedPackage));
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
	if (!Engine::Files::TryReadAllBytes(path, fileBytes, outErrorMessage))
	{
		return false;
	}

	ShaderPackageByteReader reader(fileBytes);
	if (!reader.Read(outPackage.m_header, outErrorMessage))
	{
		return false;
	}

	if (!outPackage.m_header.Matches(kCookedShaderPackageMagic, kCookedShaderPackageVersion))
	{
		if (outPackage.m_header.Magic == kCookedShaderPackageMagic && outPackage.m_header.Version == kCookedShaderPackageVersionV1)
		{
			outErrorMessage = std::format(
			    "Cooked shader package '{}' is version {} (legacy v1, pre-reflection). Recook to version {} (Phase 2b adds shader "
			    "reflection and backend identity).",
			    path.string(),
			    kCookedShaderPackageVersionV1,
			    kCookedShaderPackageVersion);
			return false;
		}

		outErrorMessage = std::format(
		    "Invalid cooked shader package header in '{}': expected magic {:08X} version {}, got magic {:08X} version {}",
		    path.string(),
		    kCookedShaderPackageMagic,
		    kCookedShaderPackageVersion,
		    outPackage.m_header.Magic,
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
    std::string& outErrorMessage)
{
	if (!package.IsValid())
	{
		outErrorMessage = "Cooked shader package payload is invalid.";
		return false;
	}

	const std::uint64_t expectedPackageKey = BuildShaderPackageKey(definition.PackageId, definition.VariantId);
	if (package.GetHeader().ShaderPackageKey != expectedPackageKey)
	{
		outErrorMessage = std::format(
		    "Cooked shader package '{}' variant '{}' failed compatibility check: field=ShaderPackageKey expected={:016X} actual={:016X}",
		    definition.PackageId,
		    definition.VariantId,
		    expectedPackageKey,
		    package.GetHeader().ShaderPackageKey);
		return false;
	}

	const std::uint64_t expectedVariantHash = BuildShaderVariantHash(definition.VariantId);
	if (package.GetHeader().VariantHash != expectedVariantHash)
	{
		outErrorMessage = std::format(
		    "Cooked shader package '{}' variant '{}' failed compatibility check: field=VariantHash expected={:016X} actual={:016X}",
		    definition.PackageId,
		    definition.VariantId,
		    expectedVariantHash,
		    package.GetHeader().VariantHash);
		return false;
	}

	if (package.GetHeader().SourceIdentityHash == 0)
	{
		outErrorMessage = std::format(
		    "Cooked shader package '{}' variant '{}' failed compatibility check: field=SourceIdentityHash actual=0",
		    definition.PackageId,
		    definition.VariantId);
		return false;
	}

	const std::uint64_t expectedBindingLayoutHash = BuildPassParameterLayoutHash(expectedBindingLayout);
	if (package.GetHeader().BindingLayoutHash != expectedBindingLayoutHash)
	{
		outErrorMessage = std::format(
		    "Cooked shader package '{}' variant '{}' failed compatibility check: field=BindingLayoutHash bindingLayout='{}' expected={:016X} actual={:016X}",
		    definition.PackageId,
		    definition.VariantId,
		    definition.BindingLayoutId != nullptr ? definition.BindingLayoutId : expectedBindingLayout.GetDebugName().c_str(),
		    expectedBindingLayoutHash,
		    package.GetHeader().BindingLayoutHash);
		return false;
	}

	if (package.GetHeader().ShaderModelMajor != static_cast<std::uint16_t>(RenderConfig::ShaderModelMajor) ||
	    package.GetHeader().ShaderModelMinor != static_cast<std::uint16_t>(RenderConfig::ShaderModelMinor))
	{
		outErrorMessage = std::format(
		    "Cooked shader package '{}' variant '{}' failed compatibility check: field=ShaderModel expected={}.{} actual={}.{}",
		    definition.PackageId,
		    definition.VariantId,
		    RenderConfig::ShaderModelMajor,
		    RenderConfig::ShaderModelMinor,
		    package.GetHeader().ShaderModelMajor,
		    package.GetHeader().ShaderModelMinor);
		return false;
	}

	if (!HasAllStages(package.GetHeader().DeclaredStages, definition.ExpectedStages))
	{
		outErrorMessage = std::format(
		    "Cooked shader package '{}' variant '{}' failed compatibility check: field=DeclaredStages expected='{}' actual='{}'",
		    definition.PackageId,
		    definition.VariantId,
		    FormatStageMask(definition.ExpectedStages),
		    FormatStageMask(package.GetHeader().DeclaredStages));
		return false;
	}

	const std::vector<PassParameterDesc>& expectedParameters = expectedBindingLayout.GetParameters();
	if (package.GetBindingRecords().size() != expectedParameters.size())
	{
		outErrorMessage = std::format(
		    "Cooked shader package '{}' variant '{}' declares {} binding records but runtime layout '{}' expects {}",
		    definition.PackageId,
		    definition.VariantId,
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
			    "Cooked shader package '{}' variant '{}' has an invalid binding name string for binding index {}",
			    definition.PackageId,
			    definition.VariantId,
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
			    "Cooked shader package '{}' variant '{}' binding record {} does not match runtime layout parameter '{}'",
			    definition.PackageId,
			    definition.VariantId,
			    parameterIndex,
			    expectedParameter.Name);
			return false;
		}
	}

	std::array<bool, static_cast<std::size_t>(ShaderStage::Count)> hasDxilBinaryForStage = {};
	for (const CookedShaderBinaryRecord& binaryRecord : package.m_binaryRecords)
	{
		if (binaryRecord.Stage == ShaderStage::Count)
		{
			outErrorMessage = std::format(
			    "Cooked shader package '{}' variant '{}' contains an invalid shader stage record",
			    definition.PackageId,
			    definition.VariantId);
			return false;
		}

		if (package.ResolveString(binaryRecord.EntryPoint).empty())
		{
			outErrorMessage = std::format(
			    "Cooked shader package '{}' variant '{}' contains an invalid entry point string",
			    definition.PackageId,
			    definition.VariantId);
			return false;
		}

		if (binaryRecord.DebugArtifact && package.ResolveString(binaryRecord.DebugArtifact).empty())
		{
			outErrorMessage = std::format(
			    "Cooked shader package '{}' variant '{}' contains an invalid debug artifact string",
			    definition.PackageId,
			    definition.VariantId);
			return false;
		}

		const ShaderBytecode bytecode = package.GetBytecode(binaryRecord);
		if (!bytecode.IsValid())
		{
			outErrorMessage = std::format(
			    "Cooked shader package '{}' variant '{}' contains an invalid bytecode blob for stage {}",
			    definition.PackageId,
			    definition.VariantId,
			    static_cast<std::uint32_t>(binaryRecord.Stage));
			return false;
		}

		if (Hash::Fnv1a64(bytecode.Data, bytecode.Size) != binaryRecord.BytecodeHash)
		{
			outErrorMessage = std::format(
			    "Cooked shader package '{}' variant '{}' failed bytecode hash validation for stage {}",
			    definition.PackageId,
			    definition.VariantId,
			    static_cast<std::uint32_t>(binaryRecord.Stage));
			return false;
		}

		if (!HasAllStages(package.GetHeader().DeclaredStages, ToShaderStageMask(binaryRecord.Stage)))
		{
			outErrorMessage = std::format(
			    "Cooked shader package '{}' variant '{}' contains a stage record outside its declared stage mask",
			    definition.PackageId,
			    definition.VariantId);
			return false;
		}

		if (binaryRecord.Format == CookedShaderBinaryFormat::Dxil)
		{
			const std::size_t stageIndex = static_cast<std::size_t>(binaryRecord.Stage);
			if (hasDxilBinaryForStage[stageIndex])
			{
				outErrorMessage = std::format(
				    "Cooked shader package '{}' variant '{}' contains more than one DXIL binary for stage {}",
				    definition.PackageId,
				    definition.VariantId,
				    stageIndex);
				return false;
			}

			hasDxilBinaryForStage[stageIndex] = true;
		}
	}

	for (const ShaderStage stage : kKnownShaderStages)
	{
		if (!HasAllStages(definition.ExpectedStages, ToShaderStageMask(stage)))
		{
			continue;
		}

		if (!hasDxilBinaryForStage[static_cast<std::size_t>(stage)])
		{
			outErrorMessage = std::format(
			    "Cooked shader package '{}' variant '{}' is missing the required DXIL binary for stage {}",
			    definition.PackageId,
			    definition.VariantId,
			    static_cast<std::uint32_t>(stage));
			return false;
		}
	}

	outErrorMessage.clear();
	return true;
}