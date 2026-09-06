#pragma once

#include "../RHIAPI.h"
#include "ShaderBytecode.h"
#include "ShaderMap.h"

#include <cstdint>
#include <filesystem>
#include <span>
#include <string_view>
#include <vector>

class SPARKLE_RHI_API CookedShaderLibrary final
{
public:
	CookedShaderLibrary() = default;
	static CookedShaderLibrary Open(const std::filesystem::path& path);

	std::uint64_t GetPublicationHash() const noexcept { return m_header.PublicationHash; }
	std::span<const CookedShaderCodeRecord> GetRecords() const noexcept { return m_records; }
	const CookedShaderCodeRecord* Find(ShaderCodeHash codeHash) const noexcept;
	ShaderBytecode GetBytecode(const CookedShaderCodeRecord& record) const noexcept;

private:
	CookedShaderLibraryHeader m_header = {};
	std::vector<CookedShaderCodeRecord> m_records;
	std::vector<std::uint8_t> m_codeBlob;
};

class SPARKLE_RHI_API GlobalShaderMap final
{
public:
	GlobalShaderMap() = default;
	static GlobalShaderMap Open(const std::filesystem::path& path, const CookedShaderLibrary& library);

	std::uint64_t GetPublicationHash() const noexcept { return m_header.PublicationHash; }
	std::span<const GlobalShaderMapEntry> GetEntries() const noexcept { return m_entries; }
	const GlobalShaderMapEntry* Find(ShaderTypeId shaderType, ShaderTarget target) const noexcept;
	std::string_view ResolveString(ShaderMapStringRef ref) const noexcept;
	std::span<const ShaderMapBindingRecord> GetBindings(const GlobalShaderMapEntry& entry) const noexcept;
	const CookedShaderReflectionRecord& GetReflection(const GlobalShaderMapEntry& entry) const noexcept;
	std::span<const CookedShaderResourceBindingRecord> GetResourceBindings() const noexcept { return m_resourceBindings; }
	std::span<const CookedShaderConstantBufferRecord> GetConstantBuffers() const noexcept { return m_constantBuffers; }
	std::span<const CookedShaderConstantBufferMemberRecord> GetConstantBufferMembers() const noexcept { return m_constantBufferMembers; }
	std::span<const CookedShaderInputElementRecord> GetInputElements() const noexcept { return m_inputElements; }
	std::span<const CookedShaderPushConstantRangeRecord> GetPushConstantRanges() const noexcept { return m_pushConstantRanges; }
	std::span<const CookedShaderSpecializationConstantRecord> GetSpecializationConstants() const noexcept
	{
		return m_specializationConstants;
	}

private:
	GlobalShaderMapHeader m_header = {};
	std::vector<GlobalShaderMapEntry> m_entries;
	std::vector<ShaderMapBindingRecord> m_bindingRecords;
	std::vector<CookedShaderReflectionRecord> m_reflectionRecords;
	std::vector<CookedShaderResourceBindingRecord> m_resourceBindings;
	std::vector<CookedShaderConstantBufferRecord> m_constantBuffers;
	std::vector<CookedShaderConstantBufferMemberRecord> m_constantBufferMembers;
	std::vector<CookedShaderInputElementRecord> m_inputElements;
	std::vector<CookedShaderPushConstantRangeRecord> m_pushConstantRanges;
	std::vector<CookedShaderSpecializationConstantRecord> m_specializationConstants;
	std::vector<std::uint8_t> m_stringTable;
};

struct ResolvedShader final
{
	const GlobalShaderMap* Map = nullptr;
	const CookedShaderLibrary* Library = nullptr;
	const GlobalShaderMapEntry* Entry = nullptr;
	const CookedShaderCodeRecord* Code = nullptr;

	bool IsValid() const noexcept { return Map != nullptr && Library != nullptr && Entry != nullptr && Code != nullptr; }
	explicit operator bool() const noexcept { return IsValid(); }
	ShaderBytecode GetBytecode() const noexcept { return IsValid() ? Library->GetBytecode(*Code) : ShaderBytecode{}; }
};
