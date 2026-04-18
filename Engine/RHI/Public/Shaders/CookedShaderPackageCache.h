#pragma once

#include "../RHIAPI.h"

#include "CookedShaderPackage.h"
#include "CookedShaderPackageUtils.h"
#include "ShaderCompileResult.h"

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

	const std::vector<CookedShaderBindingRecord>& GetBindingRecords() const noexcept { return m_bindingRecords; }
	const std::vector<CookedShaderSpecializationInputRecord>& GetSpecializationInputs() const noexcept { return m_specializationInputs; }

	const CookedShaderBinaryRecord* FindBinaryRecord(ShaderStage stage, CookedShaderBinaryFormat format = CookedShaderBinaryFormat::Dxil)
	    const noexcept;
	ShaderBytecode GetStageBytecode(ShaderStage stage, CookedShaderBinaryFormat format = CookedShaderBinaryFormat::Dxil) const noexcept;
	ShaderBytecode GetBytecode(const CookedShaderBinaryRecord& record) const noexcept;
	std::string_view ResolveString(CookedShaderStringRef ref) const noexcept;

  private:
	friend class CookedShaderPackageCache;

	bool ContainsStringRef(CookedShaderStringRef ref) const noexcept;
	bool ContainsBlobRef(CookedShaderBlobRef ref) const noexcept;

	CookedShaderPackageHeader m_header = {};
	std::vector<CookedShaderBinaryRecord> m_binaryRecords;
	std::vector<CookedShaderBindingRecord> m_bindingRecords;
	std::vector<CookedShaderSpecializationInputRecord> m_specializationInputs;
	std::vector<std::uint8_t> m_stringTable;
	std::vector<std::uint8_t> m_binaryBlob;
	bool m_isValid = false;
};

class SPARKLE_RHI_API CookedShaderPackageCache final
{
  public:
	bool LoadPackage(
	    const ShaderPackageDefinition& definition,
	    const PassParameterLayout& expectedBindingLayout,
	    std::string& outErrorMessage,
	    const LoadedShaderPackage*& outPackage);

  private:
	static bool LoadPackageFromFile(const std::filesystem::path& path, LoadedShaderPackage& outPackage, std::string& outErrorMessage);
	static bool ValidatePackage(
	    const LoadedShaderPackage& package,
	    const ShaderPackageDefinition& definition,
	    const PassParameterLayout& expectedBindingLayout,
	    std::string& outErrorMessage);

	std::unordered_map<std::uint64_t, std::unique_ptr<LoadedShaderPackage>> m_packages;
};