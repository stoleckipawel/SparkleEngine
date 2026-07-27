#pragma once

#include "../RHIAPI.h"
#include "CookedShaderPackageIdentity.h"
#include "LoadedShaderPackage.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

class PassParameterLayout;

struct CookedShaderPackageLoadReport final
{
	std::uint64_t PackageKey = 0;
	std::filesystem::path PackagePath;
	std::uint64_t CacheGeneration = 0;
	std::uint64_t ElapsedMicroseconds = 0;
	std::uint32_t BinaryRecordCount = 0;
	std::uint32_t PipelineLayoutRecordCount = 0;
	std::uint32_t ReflectionRecordCount = 0;
	bool WasCacheHit = false;
	bool WasReload = false;
	bool Succeeded = false;
};

class SPARKLE_RHI_API CookedShaderPackageCache final
{
  public:
	std::uint64_t GetGeneration() const noexcept { return m_generation; }
	const CookedShaderPackageLoadReport& GetLastLoadReport() const noexcept { return m_lastLoadReport; }
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
	CookedShaderPackageLoadReport m_lastLoadReport;
	std::uint64_t m_generation = 1;
};
