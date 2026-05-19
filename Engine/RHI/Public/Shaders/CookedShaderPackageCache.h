#pragma once

#include "../RHIAPI.h"

#include "CookedShaderPackage.h"
#include "CookedShaderPackageUtils.h"
#include "ShaderBytecode.h"

#include "../RayTracing/RhiRayTracingDesc.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class PassParameterLayout;

class SPARKLE_RHI_API LoadedShaderPackage final
{
  public:
	LoadedShaderPackage() = default;

	bool IsValid() const noexcept { return m_isValid; }
	explicit operator bool() const noexcept { return IsValid(); }

	const CookedShaderPackageHeader& GetHeader() const noexcept { return m_header; }
	std::uint64_t GetPackageKey() const noexcept { return m_header.ShaderPackageKey; }

	const std::vector<CookedShaderBinaryRecord>& GetBinaryRecords() const noexcept { return m_binaryRecords; }
	const std::vector<CookedShaderBindingRecord>& GetBindingRecords() const noexcept { return m_bindingRecords; }
	const std::vector<CookedShaderPipelineLayoutRecord>& GetPipelineLayoutRecords() const noexcept { return m_pipelineLayoutRecords; }
	const std::vector<CookedShaderSpecializationInputRecord>& GetSpecializationInputs() const noexcept { return m_specializationInputs; }
	const std::vector<CookedShaderRayTracingExportRecord>& GetRayTracingExports() const noexcept { return m_rayTracingExports; }
	const std::vector<CookedShaderRayTracingHitGroupRecord>& GetRayTracingHitGroups() const noexcept { return m_rayTracingHitGroups; }
	const std::vector<CookedShaderRayTracingLocalParameterRecord>& GetRayTracingLocalParameters() const noexcept
	{
		return m_rayTracingLocalParameters;
	}

	// v2 reflection accessors. ReflectionRecords is parallel to BinaryRecords;
	// the typed arrays are package-wide, indexed by the offsets in each
	// CookedShaderReflectionRecord.
	const std::vector<CookedShaderReflectionRecord>& GetReflectionRecords() const noexcept { return m_reflectionRecords; }
	const std::vector<CookedShaderResourceBindingRecord>& GetResourceBindings() const noexcept { return m_resourceBindings; }
	const std::vector<CookedShaderConstantBufferRecord>& GetConstantBuffers() const noexcept { return m_constantBuffers; }
	const std::vector<CookedShaderConstantBufferMemberRecord>& GetConstantBufferMembers() const noexcept { return m_constantBufferMembers; }
	const std::vector<CookedShaderInputElementRecord>& GetInputElements() const noexcept { return m_inputElements; }
	const std::vector<CookedShaderPushConstantRangeRecord>& GetPushConstantRanges() const noexcept { return m_pushConstantRanges; }
	const std::vector<CookedShaderSpecializationConstantRecord>& GetSpecializationConstants() const noexcept
	{
		return m_specializationConstants;
	}

	const CookedShaderBinaryRecord* FindBinaryRecord(ShaderStage stage, CookedShaderBinaryFormat format) const noexcept;
	ShaderBytecode GetStageBytecode(ShaderStage stage, CookedShaderBinaryFormat format) const noexcept;
	ShaderBytecode GetBytecode(const CookedShaderBinaryRecord& record) const noexcept;
	std::string_view ResolveString(CookedShaderStringRef ref) const noexcept;
	bool ValidateRayTracingLibraryMetadata(
	    const RhiRayTracingCapabilities& capabilities,
	    CookedShaderBinaryFormat requiredBinaryFormat,
	    std::string& outErrorMessage) const;

  private:
	friend class CookedShaderPackageCache;

	bool ContainsStringRef(CookedShaderStringRef ref) const noexcept;
	bool ContainsBlobRef(CookedShaderBlobRef ref) const noexcept;

	CookedShaderPackageHeader m_header = {};
	std::vector<CookedShaderBinaryRecord> m_binaryRecords;
	std::vector<CookedShaderBindingRecord> m_bindingRecords;
	std::vector<CookedShaderPipelineLayoutRecord> m_pipelineLayoutRecords;
	std::vector<CookedShaderSpecializationInputRecord> m_specializationInputs;
	std::vector<CookedShaderRayTracingExportRecord> m_rayTracingExports;
	std::vector<CookedShaderRayTracingHitGroupRecord> m_rayTracingHitGroups;
	std::vector<CookedShaderRayTracingLocalParameterRecord> m_rayTracingLocalParameters;
	std::vector<CookedShaderReflectionRecord> m_reflectionRecords;
	std::vector<CookedShaderResourceBindingRecord> m_resourceBindings;
	std::vector<CookedShaderConstantBufferRecord> m_constantBuffers;
	std::vector<CookedShaderConstantBufferMemberRecord> m_constantBufferMembers;
	std::vector<CookedShaderInputElementRecord> m_inputElements;
	std::vector<CookedShaderPushConstantRangeRecord> m_pushConstantRanges;
	std::vector<CookedShaderSpecializationConstantRecord> m_specializationConstants;
	std::vector<std::uint8_t> m_stringTable;
	std::vector<std::uint8_t> m_binaryBlob;
	bool m_isValid = false;
};

class SPARKLE_RHI_API CookedShaderPackageCache final
{
  public:
	std::uint64_t GetGeneration() const noexcept { return m_generation; }
	void Clear() noexcept;
	void ReplaceWith(CookedShaderPackageCache&& replacement) noexcept;

	bool LoadPackage(
	    const ShaderPackageDefinition& definition,
	    const PassParameterLayout& expectedBindingLayout,
	    CookedShaderBinaryFormat requiredBinaryFormat,
	    std::string& outErrorMessage,
	    const LoadedShaderPackage*& outPackage);
	bool ReloadPackage(
	    const ShaderPackageDefinition& definition,
	    const PassParameterLayout& expectedBindingLayout,
	    CookedShaderBinaryFormat requiredBinaryFormat,
	    std::string& outErrorMessage,
	    const LoadedShaderPackage*& outPackage);

  private:
	static bool LoadPackageFromFile(const std::filesystem::path& path, LoadedShaderPackage& outPackage, std::string& outErrorMessage);
	static bool ValidatePackage(
	    const LoadedShaderPackage& package,
	    const ShaderPackageDefinition& definition,
	    const PassParameterLayout& expectedBindingLayout,
	    CookedShaderBinaryFormat requiredBinaryFormat,
	    std::string& outErrorMessage);

	std::unordered_map<std::uint64_t, std::unique_ptr<LoadedShaderPackage>> m_packages;
	std::uint64_t m_generation = 1;
};