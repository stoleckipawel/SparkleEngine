#include "PCH.h"

#include "Shaders/GlobalShaderMap.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Files/BinarySpanReader.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Hash/HashUtils.h"

#include <algorithm>
#include <format>
#include <limits>
#include <string>

ShaderFeatureFlags operator|(ShaderFeatureFlags lhs, ShaderFeatureFlags rhs) noexcept
{
	return static_cast<ShaderFeatureFlags>(static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

ShaderFeatureFlags& operator|=(ShaderFeatureFlags& lhs, ShaderFeatureFlags rhs) noexcept
{
	lhs = lhs | rhs;
	return lhs;
}

bool HasShaderFeature(ShaderFeatureFlags value, ShaderFeatureFlags flag) noexcept
{
	return (static_cast<std::uint32_t>(value) & static_cast<std::uint32_t>(flag)) == static_cast<std::uint32_t>(flag);
}

class ShaderMapValidation final
{
public:
	template <typename T> static bool ContainsRange(std::span<const T> values, std::uint32_t offset, std::uint32_t count) noexcept
	{
		return offset <= values.size() && count <= values.size() - offset;
	}

	static bool ContainsString(std::span<const std::uint8_t> table, ShaderMapStringRef ref) noexcept
	{
		return ContainsRange(table, ref.OffsetInBytes, ref.SizeInBytes);
	}

	static bool ContainsCode(std::span<const std::uint8_t> blob, ShaderCodeBlobRef ref) noexcept
	{
		return ref.IsValid() && ContainsRange(blob, ref.OffsetInBytes, ref.SizeInBytes);
	}

	static void ValidateReflection(
	    const CookedShaderReflectionRecord& reflection,
	    std::span<const CookedShaderResourceBindingRecord> resources,
	    std::span<const CookedShaderConstantBufferRecord> constantBuffers,
	    std::span<const CookedShaderConstantBufferMemberRecord> constantBufferMembers,
	    std::span<const CookedShaderInputElementRecord> inputs,
	    std::span<const CookedShaderPushConstantRangeRecord> pushConstants,
	    std::span<const CookedShaderSpecializationConstantRecord> specializationConstants,
	    std::span<const std::uint8_t> strings)
	{
		if (!ContainsRange(resources, reflection.ResourceBindingOffset, reflection.ResourceBindingCount)
		    || !ContainsRange(constantBuffers, reflection.ConstantBufferOffset, reflection.ConstantBufferCount)
		    || !ContainsRange(inputs, reflection.InputElementOffset, reflection.InputElementCount)
		    || !ContainsRange(pushConstants, reflection.PushConstantRangeOffset, reflection.PushConstantRangeCount)
		    || !ContainsRange(
		        specializationConstants,
		        reflection.SpecializationConstantOffset,
		        reflection.SpecializationConstantCount))
		{
			throw Diagnostics::Error("Global shader map contains an out-of-range reflection record.");
		}

		for (const CookedShaderResourceBindingRecord& resource : resources.subspan(reflection.ResourceBindingOffset, reflection.ResourceBindingCount))
		{
			const bool validConstantBuffer = resource.ConstantBufferIndex == kCookedShaderReflectionInvalidIndex
			    || (resource.ConstantBufferIndex >= reflection.ConstantBufferOffset
			        && resource.ConstantBufferIndex < reflection.ConstantBufferOffset + reflection.ConstantBufferCount);
			if (!ContainsRange(strings, resource.NameOffsetInBytes, resource.NameSizeInBytes) || resource.NameSizeInBytes == 0
			    || resource.ArrayCount == 0 || !validConstantBuffer)
			{
				throw Diagnostics::Error("Global shader map contains an invalid reflected resource binding.");
			}
		}
		for (const CookedShaderConstantBufferRecord& buffer : constantBuffers.subspan(reflection.ConstantBufferOffset, reflection.ConstantBufferCount))
		{
			if (!ContainsRange(strings, buffer.NameOffsetInBytes, buffer.NameSizeInBytes) || buffer.NameSizeInBytes == 0
			    || !ContainsRange(constantBufferMembers, buffer.MemberOffset, buffer.MemberCount))
			{
				throw Diagnostics::Error("Global shader map contains an invalid reflected constant buffer.");
			}
			for (const CookedShaderConstantBufferMemberRecord& member : constantBufferMembers.subspan(buffer.MemberOffset, buffer.MemberCount))
			{
				if (!ContainsRange(strings, member.NameOffsetInBytes, member.NameSizeInBytes) || member.NameSizeInBytes == 0
				    || member.ArrayCount == 0 || member.OffsetInBytes > buffer.SizeInBytes
				    || member.SizeInBytes > buffer.SizeInBytes - member.OffsetInBytes)
				{
					throw Diagnostics::Error("Global shader map contains an invalid reflected constant-buffer member.");
				}
			}
		}
		for (const CookedShaderInputElementRecord& input : inputs.subspan(reflection.InputElementOffset, reflection.InputElementCount))
		{
			if (!ContainsRange(strings, input.SemanticOffsetInBytes, input.SemanticSizeInBytes) || input.SemanticSizeInBytes == 0)
			{
				throw Diagnostics::Error("Global shader map contains an invalid reflected input element.");
			}
		}
		for (const CookedShaderSpecializationConstantRecord& value : specializationConstants.subspan(
		         reflection.SpecializationConstantOffset,
		         reflection.SpecializationConstantCount))
		{
			if (!ContainsRange(strings, value.NameOffsetInBytes, value.NameSizeInBytes) || value.NameSizeInBytes == 0)
			{
				throw Diagnostics::Error("Global shader map contains an invalid reflected specialization constant.");
			}
		}
	}
};

CookedShaderLibrary CookedShaderLibrary::Open(const std::filesystem::path& path)
{
	std::vector<std::uint8_t> fileBytes;
	std::string error;
	if (!Files::TryReadAllBytes(path, fileBytes, error))
	{
		throw Diagnostics::Error(std::move(error));
	}

	CookedShaderLibrary library;
	Files::BinarySpanReader reader(fileBytes);
	if (!reader.ReadValue(library.m_header, error) || library.m_header.Magic != kCookedShaderLibraryMagic)
	{
		throw Diagnostics::Error(std::format("Invalid cooked shader library header in '{}'.", path.string()));
	}
	if (library.m_header.PublicationHash == 0 || !reader.ReadArray(library.m_header.RecordCount, library.m_records, error)
	    || !reader.ReadArray(library.m_header.CodeBlobSizeInBytes, library.m_codeBlob, error))
	{
		throw Diagnostics::Error(error.empty() ? "Cooked shader library has no publication identity." : std::move(error));
	}
	if (reader.GetRemainingByteCount() != 0)
	{
		throw Diagnostics::Error(std::format("Cooked shader library '{}' contains trailing bytes.", path.string()));
	}

	ShaderCodeHash previousHash = 0;
	for (const CookedShaderCodeRecord& record : library.m_records)
	{
		if (record.CodeHash == 0 || (previousHash != 0 && record.CodeHash <= previousHash))
		{
			throw Diagnostics::Error("Cooked shader library index is not strictly ordered by unique code hash.");
		}
		if (!ShaderMapValidation::ContainsCode(library.m_codeBlob, record.Code))
		{
			throw Diagnostics::Error("Cooked shader library contains a truncated code record.");
		}
		const std::uint8_t* code = library.m_codeBlob.data() + record.Code.OffsetInBytes;
		if (Hash::Fnv1a64(code, record.Code.SizeInBytes) != record.CodeHash)
		{
			throw Diagnostics::Error(
			    std::format("Cooked shader library code hash {} does not match its bytes.", Formatting::FormatHexUInt64(record.CodeHash)));
		}
		previousHash = record.CodeHash;
	}
	return library;
}

const CookedShaderCodeRecord* CookedShaderLibrary::Find(ShaderCodeHash codeHash) const noexcept
{
	const auto found = std::lower_bound(
	    m_records.begin(),
	    m_records.end(),
	    codeHash,
	    [](const CookedShaderCodeRecord& record, ShaderCodeHash value) { return record.CodeHash < value; });
	return found != m_records.end() && found->CodeHash == codeHash ? &*found : nullptr;
}

ShaderBytecode CookedShaderLibrary::GetBytecode(const CookedShaderCodeRecord& record) const noexcept
{
	if (!ShaderMapValidation::ContainsCode(m_codeBlob, record.Code))
	{
		return {};
	}
	return ShaderBytecode{m_codeBlob.data() + record.Code.OffsetInBytes, record.Code.SizeInBytes};
}

GlobalShaderMap GlobalShaderMap::Open(const std::filesystem::path& path, const CookedShaderLibrary& library)
{
	std::vector<std::uint8_t> fileBytes;
	std::string error;
	if (!Files::TryReadAllBytes(path, fileBytes, error))
	{
		throw Diagnostics::Error(std::move(error));
	}

	GlobalShaderMap map;
	Files::BinarySpanReader reader(fileBytes);
	if (!reader.ReadValue(map.m_header, error) || map.m_header.Magic != kGlobalShaderMapMagic)
	{
		throw Diagnostics::Error(std::format("Invalid global shader map header in '{}'.", path.string()));
	}
	if (map.m_header.PublicationHash == 0 || map.m_header.PublicationHash != library.GetPublicationHash())
	{
		throw Diagnostics::Error("Global shader map and cooked shader library do not belong to the same publication.");
	}
	if (!reader.ReadArray(map.m_header.EntryCount, map.m_entries, error)
	    || !reader.ReadArray(map.m_header.BindingRecordCount, map.m_bindingRecords, error)
	    || !reader.ReadArray(map.m_header.ReflectionRecordCount, map.m_reflectionRecords, error)
	    || !reader.ReadArray(map.m_header.ResourceBindingRecordCount, map.m_resourceBindings, error)
	    || !reader.ReadArray(map.m_header.ConstantBufferRecordCount, map.m_constantBuffers, error)
	    || !reader.ReadArray(map.m_header.ConstantBufferMemberRecordCount, map.m_constantBufferMembers, error)
	    || !reader.ReadArray(map.m_header.InputElementRecordCount, map.m_inputElements, error)
	    || !reader.ReadArray(map.m_header.PushConstantRangeRecordCount, map.m_pushConstantRanges, error)
	    || !reader.ReadArray(map.m_header.SpecializationConstantRecordCount, map.m_specializationConstants, error)
	    || !reader.ReadArray(map.m_header.StringTableSizeInBytes, map.m_stringTable, error))
	{
		throw Diagnostics::Error(std::move(error));
	}
	if (reader.GetRemainingByteCount() != 0)
	{
		throw Diagnostics::Error(std::format("Global shader map '{}' contains trailing bytes.", path.string()));
	}

	ShaderTypeId previousType = 0;
	ShaderTarget previousTarget = kDefaultShaderTarget;
	bool hasPrevious = false;
	for (const GlobalShaderMapEntry& entry : map.m_entries)
	{
		const bool ordered = !hasPrevious || previousType < entry.ShaderType
		    || (previousType == entry.ShaderType && static_cast<std::uint16_t>(previousTarget) < static_cast<std::uint16_t>(entry.Target));
		if (entry.ShaderType == 0 || entry.CodeHash == 0 || entry.ParameterSignature == 0 || entry.Stage >= ShaderStage::Count
		    || !IsShaderTarget(entry.Target) || !ordered)
		{
			throw Diagnostics::Error("Global shader map contains an invalid or duplicate logical key.");
		}
		if (entry.BinaryFormat != GetShaderBinaryFormat(entry.Target) || library.Find(entry.CodeHash) == nullptr)
		{
			throw Diagnostics::Error("Global shader map entry references missing or incompatible shader code.");
		}
		if (!ShaderMapValidation::ContainsString(map.m_stringTable, entry.ShaderName)
		    || !ShaderMapValidation::ContainsString(map.m_stringTable, entry.EntryPoint)
		    || !ShaderMapValidation::ContainsString(map.m_stringTable, entry.BackendName)
		    || !ShaderMapValidation::ContainsString(map.m_stringTable, entry.CodegenTarget)
		    || !ShaderMapValidation::ContainsRange(
		        std::span<const ShaderMapBindingRecord>(map.m_bindingRecords),
		        entry.BindingRecordOffset,
		        entry.BindingRecordCount)
		    || entry.ReflectionRecordIndex >= map.m_reflectionRecords.size())
		{
			throw Diagnostics::Error("Global shader map entry contains an out-of-range metadata reference.");
		}
		ShaderMapValidation::ValidateReflection(
		    map.m_reflectionRecords[entry.ReflectionRecordIndex],
		    map.m_resourceBindings,
		    map.m_constantBuffers,
		    map.m_constantBufferMembers,
		    map.m_inputElements,
		    map.m_pushConstantRanges,
		    map.m_specializationConstants,
		    map.m_stringTable);
		for (const ShaderMapBindingRecord& binding : std::span<const ShaderMapBindingRecord>(map.m_bindingRecords)
		         .subspan(entry.BindingRecordOffset, entry.BindingRecordCount))
		{
			if (!ShaderMapValidation::ContainsString(map.m_stringTable, binding.Name) || !binding.Name.IsValid()
			    || binding.ArrayCount == 0)
			{
				throw Diagnostics::Error("Global shader map entry contains an invalid parameter binding.");
			}
		}
		previousType = entry.ShaderType;
		previousTarget = entry.Target;
		hasPrevious = true;
	}
	return map;
}

const GlobalShaderMapEntry* GlobalShaderMap::Find(ShaderTypeId shaderType, ShaderTarget target) const noexcept
{
	const auto found = std::lower_bound(
	    m_entries.begin(),
	    m_entries.end(),
	    std::pair{shaderType, target},
	    [](const GlobalShaderMapEntry& entry, const std::pair<ShaderTypeId, ShaderTarget>& key)
	    {
		    return entry.ShaderType < key.first || (entry.ShaderType == key.first && entry.Target < key.second);
	    });
	return found != m_entries.end() && found->ShaderType == shaderType && found->Target == target ? &*found : nullptr;
}

std::string_view GlobalShaderMap::ResolveString(ShaderMapStringRef ref) const noexcept
{
	if (!ShaderMapValidation::ContainsString(m_stringTable, ref))
	{
		return {};
	}
	return {reinterpret_cast<const char*>(m_stringTable.data() + ref.OffsetInBytes), ref.SizeInBytes};
}

std::span<const ShaderMapBindingRecord> GlobalShaderMap::GetBindings(const GlobalShaderMapEntry& entry) const noexcept
{
	if (!ShaderMapValidation::ContainsRange(
	        std::span<const ShaderMapBindingRecord>(m_bindingRecords),
	        entry.BindingRecordOffset,
	        entry.BindingRecordCount))
	{
		return {};
	}
	return std::span<const ShaderMapBindingRecord>(m_bindingRecords).subspan(entry.BindingRecordOffset, entry.BindingRecordCount);
}

const CookedShaderReflectionRecord& GlobalShaderMap::GetReflection(const GlobalShaderMapEntry& entry) const noexcept
{
	return m_reflectionRecords[entry.ReflectionRecordIndex];
}
