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
#include <string>
#include <vector>

namespace
{
	const char* FormatCookedShaderBinaryFormat(CookedShaderBinaryFormat format) noexcept
	{
		switch (format)
		{
			case CookedShaderBinaryFormat::Dxil:
				return "DXIL";
			case CookedShaderBinaryFormat::SpirV:
				return "SPIR-V";
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

	const char* FormatCookedShaderResourceKind(CookedShaderResourceKind kind) noexcept
	{
		switch (kind)
		{
			case CookedShaderResourceKind::ConstantBuffer:
				return "ConstantBuffer";
			case CookedShaderResourceKind::Texture:
				return "Texture";
			case CookedShaderResourceKind::StructuredBuffer:
				return "StructuredBuffer";
			case CookedShaderResourceKind::ByteAddressBuffer:
				return "ByteAddressBuffer";
			case CookedShaderResourceKind::TypedBuffer:
				return "TypedBuffer";
			case CookedShaderResourceKind::RWTexture:
				return "RWTexture";
			case CookedShaderResourceKind::RWStructuredBuffer:
				return "RWStructuredBuffer";
			case CookedShaderResourceKind::RWByteAddressBuffer:
				return "RWByteAddressBuffer";
			case CookedShaderResourceKind::RWTypedBuffer:
				return "RWTypedBuffer";
			case CookedShaderResourceKind::Sampler:
				return "Sampler";
			case CookedShaderResourceKind::AccelerationStructure:
				return "AccelerationStructure";
			case CookedShaderResourceKind::PushConstantBlock:
				return "PushConstantBlock";
			case CookedShaderResourceKind::Unknown:
			default:
				return "Unknown";
		}
	}

	const char* FormatCookedShaderResourceDimension(CookedShaderResourceDimension dimension) noexcept
	{
		switch (dimension)
		{
			case CookedShaderResourceDimension::Buffer:
				return "Buffer";
			case CookedShaderResourceDimension::Texture1D:
				return "Texture1D";
			case CookedShaderResourceDimension::Texture1DArray:
				return "Texture1DArray";
			case CookedShaderResourceDimension::Texture2D:
				return "Texture2D";
			case CookedShaderResourceDimension::Texture2DArray:
				return "Texture2DArray";
			case CookedShaderResourceDimension::Texture2DMS:
				return "Texture2DMS";
			case CookedShaderResourceDimension::Texture2DMSArray:
				return "Texture2DMSArray";
			case CookedShaderResourceDimension::Texture3D:
				return "Texture3D";
			case CookedShaderResourceDimension::TextureCube:
				return "TextureCube";
			case CookedShaderResourceDimension::TextureCubeArray:
				return "TextureCubeArray";
			case CookedShaderResourceDimension::Unknown:
			default:
				return "Unknown";
		}
	}

	const char* FormatShaderStageVisibility(ShaderStageVisibility visibility) noexcept
	{
		switch (visibility)
		{
			case ShaderStageVisibility::None:
				return "None";
			case ShaderStageVisibility::Vertex:
				return "Vertex";
			case ShaderStageVisibility::Pixel:
				return "Pixel";
			case ShaderStageVisibility::Compute:
				return "Compute";
			case ShaderStageVisibility::AllGraphics:
				return "AllGraphics";
			case ShaderStageVisibility::All:
				return "All";
			default:
				return "Unknown";
		}
	}

	const char* FormatShaderParameterSemanticKind(ShaderParameterSemanticKind kind) noexcept
	{
		switch (kind)
		{
			case ShaderParameterSemanticKind::ReadTexture:
				return "ReadTexture";
			case ShaderParameterSemanticKind::ReadBuffer:
				return "ReadBuffer";
			case ShaderParameterSemanticKind::RWTexture:
				return "RWTexture";
			case ShaderParameterSemanticKind::RWBuffer:
				return "RWBuffer";
			case ShaderParameterSemanticKind::RenderTarget:
				return "RenderTarget";
			case ShaderParameterSemanticKind::DepthTarget:
				return "DepthTarget";
			case ShaderParameterSemanticKind::UniformData:
				return "UniformData";
			case ShaderParameterSemanticKind::SamplerSet:
				return "SamplerSet";
			case ShaderParameterSemanticKind::AccelerationStructure:
				return "AccelerationStructure";
			default:
				return "Unknown";
		}
	}

	std::string FormatExpectedParameterList(const std::vector<PassParameterDesc>& expectedParameters)
	{
		if (expectedParameters.empty())
		{
			return "<none>";
		}

		std::string result;
		for (const PassParameterDesc& parameter : expectedParameters)
		{
			if (!result.empty())
			{
				result += "; ";
			}

			result += std::format(
			    "{}(shader='{}', kind={}, visibility={}, size={}, array={})",
			    parameter.Name,
			    parameter.GetShaderName(),
			    FormatShaderParameterSemanticKind(parameter.Kind),
			    FormatShaderStageVisibility(parameter.Visibility),
			    parameter.ValueSizeInBytes,
			    parameter.ArrayCount);
		}

		return result;
	}

	std::string FormatReflectedBindingList(
	    const LoadedShaderPackage& package,
	    const ShaderPackageDefinition& definition,
	    CookedShaderBinaryFormat requiredBinaryFormat)
	{
		const std::vector<CookedShaderBinaryRecord>& binaryRecords = package.GetBinaryRecords();
		const std::vector<CookedShaderReflectionRecord>& reflectionRecords = package.GetReflectionRecords();
		const std::vector<CookedShaderResourceBindingRecord>& resourceBindings = package.GetResourceBindings();

		std::string result;
		for (std::size_t reflectionIndex = 0; reflectionIndex < reflectionRecords.size() && reflectionIndex < binaryRecords.size();
		     ++reflectionIndex)
		{
			const CookedShaderBinaryRecord& binaryRecord = binaryRecords[reflectionIndex];
			if (binaryRecord.Format != requiredBinaryFormat ||
			    !HasAllStages(definition.ExpectedStages, ToShaderStageMask(binaryRecord.Stage)))
			{
				continue;
			}

			const CookedShaderReflectionRecord& reflection = reflectionRecords[reflectionIndex];
			for (std::uint32_t resourceIndex = 0; resourceIndex < reflection.ResourceBindingCount; ++resourceIndex)
			{
				const std::uint32_t bindingIndex = reflection.ResourceBindingOffset + resourceIndex;
				if (bindingIndex >= resourceBindings.size())
				{
					if (!result.empty())
					{
						result += "; ";
					}
					result += std::format("{}:<out-of-range:{}>", GetShaderStagePrefix(binaryRecord.Stage), bindingIndex);
					continue;
				}

				const CookedShaderResourceBindingRecord& resourceBinding = resourceBindings[bindingIndex];
				const std::string_view resourceName =
				    package.ResolveString(CookedShaderStringRef{resourceBinding.NameOffsetInBytes, resourceBinding.NameSizeInBytes});
				if (!result.empty())
				{
					result += "; ";
				}

				result += std::format(
				    "{}:{}('{}', dim={}, space={}, slot={}, array={}, size={})",
				    GetShaderStagePrefix(binaryRecord.Stage),
				    FormatCookedShaderResourceKind(resourceBinding.Kind),
				    resourceName.empty() ? std::string_view{"<invalid>"} : resourceName,
				    FormatCookedShaderResourceDimension(resourceBinding.Dimension),
				    resourceBinding.Set,
				    resourceBinding.Slot,
				    resourceBinding.ArrayCount,
				    resourceBinding.SizeInBytes);
			}
		}

		return result.empty() ? "<none>" : result;
	}

	std::string AddBindingDiagnostics(
	    std::string message,
	    const LoadedShaderPackage& package,
	    const ShaderPackageDefinition& definition,
	    const std::vector<PassParameterDesc>& expectedParameters,
	    CookedShaderBinaryFormat requiredBinaryFormat)
	{
		message += std::format(
		    " Expected runtime bindings=[{}]. Reflected {} bindings=[{}].",
		    FormatExpectedParameterList(expectedParameters),
		    FormatCookedShaderBinaryFormat(requiredBinaryFormat),
		    FormatReflectedBindingList(package, definition, requiredBinaryFormat));
		return message;
	}

	bool ResourceKindMatchesSemantic(CookedShaderResourceKind resourceKind, ShaderParameterSemanticKind semanticKind) noexcept
	{
		switch (semanticKind)
		{
			case ShaderParameterSemanticKind::UniformData:
				return resourceKind == CookedShaderResourceKind::ConstantBuffer ||
				       resourceKind == CookedShaderResourceKind::PushConstantBlock;
			case ShaderParameterSemanticKind::ReadTexture:
				return resourceKind == CookedShaderResourceKind::Texture;
			case ShaderParameterSemanticKind::ReadBuffer:
				return resourceKind == CookedShaderResourceKind::StructuredBuffer ||
				       resourceKind == CookedShaderResourceKind::ByteAddressBuffer || resourceKind == CookedShaderResourceKind::TypedBuffer;
			case ShaderParameterSemanticKind::RWTexture:
			case ShaderParameterSemanticKind::RenderTarget:
			case ShaderParameterSemanticKind::DepthTarget:
				return resourceKind == CookedShaderResourceKind::RWTexture;
			case ShaderParameterSemanticKind::RWBuffer:
				return resourceKind == CookedShaderResourceKind::RWStructuredBuffer ||
				       resourceKind == CookedShaderResourceKind::RWByteAddressBuffer ||
				       resourceKind == CookedShaderResourceKind::RWTypedBuffer;
			case ShaderParameterSemanticKind::SamplerSet:
				return resourceKind == CookedShaderResourceKind::Sampler;
			case ShaderParameterSemanticKind::AccelerationStructure:
				return resourceKind == CookedShaderResourceKind::AccelerationStructure;
		}

		return false;
	}

	bool HasMatchingExpectedParameter(
	    std::string_view resourceName,
	    CookedShaderResourceKind resourceKind,
	    ShaderStage stage,
	    const std::vector<PassParameterDesc>& expectedParameters) noexcept
	{
		const ShaderStageMask stageMask = ToShaderStageMask(stage);
		for (const PassParameterDesc& expectedParameter : expectedParameters)
		{
			if (!HasAllStages(ToPackageStageMask(expectedParameter.Visibility), stageMask))
			{
				continue;
			}

			if (expectedParameter.GetShaderName() != resourceName)
			{
				continue;
			}

			if (ResourceKindMatchesSemantic(resourceKind, expectedParameter.Kind))
			{
				return true;
			}
		}

		return false;
	}

	bool ValidateReflectedBindings(
	    const LoadedShaderPackage& package,
	    const ShaderPackageDefinition& definition,
	    const std::vector<PassParameterDesc>& expectedParameters,
	    CookedShaderBinaryFormat requiredBinaryFormat,
	    std::string& outErrorMessage)
	{
		const std::vector<CookedShaderBinaryRecord>& binaryRecords = package.GetBinaryRecords();
		const std::vector<CookedShaderReflectionRecord>& reflectionRecords = package.GetReflectionRecords();
		const std::vector<CookedShaderResourceBindingRecord>& resourceBindings = package.GetResourceBindings();

		if (reflectionRecords.empty())
		{
			return true;
		}

		if (reflectionRecords.size() != binaryRecords.size())
		{
			outErrorMessage = std::format(
			    "Cooked shader package '{}' failed compatibility check: reflection count {} does not match binary count {}",
			    definition.PackageId,
			    reflectionRecords.size(),
			    binaryRecords.size());
			return false;
		}

		for (std::size_t reflectionIndex = 0; reflectionIndex < reflectionRecords.size(); ++reflectionIndex)
		{
			const CookedShaderBinaryRecord& binaryRecord = binaryRecords[reflectionIndex];
			if (binaryRecord.Format != requiredBinaryFormat)
			{
				continue;
			}

			if (!HasAllStages(definition.ExpectedStages, ToShaderStageMask(binaryRecord.Stage)))
			{
				continue;
			}

			const CookedShaderReflectionRecord& reflection = reflectionRecords[reflectionIndex];
			for (std::uint32_t resourceIndex = 0; resourceIndex < reflection.ResourceBindingCount; ++resourceIndex)
			{
				const std::uint32_t bindingIndex = reflection.ResourceBindingOffset + resourceIndex;
				if (bindingIndex >= resourceBindings.size())
				{
					outErrorMessage = std::format(
					    "Cooked shader package '{}' failed compatibility check: reflection resource binding index {} is out of range",
					    definition.PackageId,
					    bindingIndex);
					return false;
				}

				const CookedShaderResourceBindingRecord& resourceBinding = resourceBindings[bindingIndex];
				const std::string_view resourceName =
				    package.ResolveString(CookedShaderStringRef{resourceBinding.NameOffsetInBytes, resourceBinding.NameSizeInBytes});
				if (resourceName.empty())
				{
					outErrorMessage = std::format(
					    "Cooked shader package '{}' failed compatibility check: reflection resource {} has an invalid name string",
					    definition.PackageId,
					    bindingIndex);
					return false;
				}

				if (!HasMatchingExpectedParameter(resourceName, resourceBinding.Kind, binaryRecord.Stage, expectedParameters))
				{
					outErrorMessage = AddBindingDiagnostics(
					    std::format(
					        "Cooked shader package '{}' failed compatibility check: stage {} reflects unexpected {} '{}' for backend {}. "
					        "Recook shaders to refresh stale package metadata.",
					        definition.PackageId,
					        static_cast<std::uint32_t>(binaryRecord.Stage),
					        FormatCookedShaderResourceKind(resourceBinding.Kind),
					        resourceName,
					        FormatCookedShaderBinaryFormat(requiredBinaryFormat)),
					    package,
					    definition,
					    expectedParameters,
					    requiredBinaryFormat);
					return false;
				}
			}
		}

		for (const PassParameterDesc& expectedParameter : expectedParameters)
		{
			bool foundMatch = false;
			for (std::size_t reflectionIndex = 0; reflectionIndex < reflectionRecords.size() && !foundMatch; ++reflectionIndex)
			{
				const CookedShaderBinaryRecord& binaryRecord = binaryRecords[reflectionIndex];
				if (binaryRecord.Format != requiredBinaryFormat)
				{
					continue;
				}

				if (!HasAllStages(definition.ExpectedStages, ToShaderStageMask(binaryRecord.Stage)) ||
				    !HasAllStages(ToPackageStageMask(expectedParameter.Visibility), ToShaderStageMask(binaryRecord.Stage)))
				{
					continue;
				}

				const CookedShaderReflectionRecord& reflection = reflectionRecords[reflectionIndex];
				for (std::uint32_t resourceIndex = 0; resourceIndex < reflection.ResourceBindingCount; ++resourceIndex)
				{
					const std::uint32_t bindingIndex = reflection.ResourceBindingOffset + resourceIndex;
					if (bindingIndex >= resourceBindings.size())
					{
						outErrorMessage = std::format(
						    "Cooked shader package '{}' failed compatibility check: reflection resource binding index {} is out of range",
						    definition.PackageId,
						    bindingIndex);
						return false;
					}

					const CookedShaderResourceBindingRecord& resourceBinding = resourceBindings[bindingIndex];
					const std::string_view resourceName =
					    package.ResolveString(CookedShaderStringRef{resourceBinding.NameOffsetInBytes, resourceBinding.NameSizeInBytes});
					if (resourceName == expectedParameter.GetShaderName() &&
					    ResourceKindMatchesSemantic(resourceBinding.Kind, expectedParameter.Kind))
					{
						foundMatch = true;
						break;
					}
				}
			}

			if (!foundMatch)
			{
				outErrorMessage = AddBindingDiagnostics(
				    std::format(
				        "Cooked shader package '{}' failed compatibility check: runtime parameter '{}' (shader='{}') is missing reflected "
				        "backend {} bindings. Recook shaders to refresh stale package metadata.",
				        definition.PackageId,
				        expectedParameter.Name,
				        expectedParameter.GetShaderName(),
				        FormatCookedShaderBinaryFormat(requiredBinaryFormat)),
				    package,
				    definition,
				    expectedParameters,
				    requiredBinaryFormat);
				return false;
			}
		}

		return true;
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
	    !reader
	         .ReadArray(outPackage.m_header.RayTracingLocalParameterRecordCount, outPackage.m_rayTracingLocalParameters, outErrorMessage) ||
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
		outErrorMessage =
		    std::format("Cooked shader package '{}' failed compatibility check: field=SourceIdentityHash actual=0", definition.PackageId);
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

	if (!ValidateReflectedBindings(package, definition, expectedParameters, requiredBinaryFormat, outErrorMessage))
	{
		return false;
	}

	std::array<bool, static_cast<std::size_t>(ShaderStage::Count)> hasRequiredBinaryForStage = {};
	for (const CookedShaderBinaryRecord& binaryRecord : package.m_binaryRecords)
	{
		if (binaryRecord.Stage == ShaderStage::Count)
		{
			outErrorMessage = std::format("Cooked shader package '{}' contains an invalid shader stage record", definition.PackageId);
			return false;
		}

		if (package.ResolveString(binaryRecord.EntryPoint).empty())
		{
			outErrorMessage = std::format("Cooked shader package '{}' contains an invalid entry point string", definition.PackageId);
			return false;
		}

		if (binaryRecord.DebugArtifact && package.ResolveString(binaryRecord.DebugArtifact).empty())
		{
			outErrorMessage = std::format("Cooked shader package '{}' contains an invalid debug artifact string", definition.PackageId);
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
			outErrorMessage =
			    std::format("Cooked shader package '{}' contains a stage record outside its declared stage mask", definition.PackageId);
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