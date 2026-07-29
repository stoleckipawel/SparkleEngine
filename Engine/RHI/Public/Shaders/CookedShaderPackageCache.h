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

enum class CookedShaderPackageLoadKind : std::uint8_t
{
	Disk,
	CacheHit,
};

struct CookedShaderPackageLoadReport final
{
	std::uint64_t PackageKey = 0;
	std::filesystem::path PackagePath;
	std::uint64_t CacheGeneration = 0;
	std::uint64_t ElapsedMicroseconds = 0;
	std::uint32_t BinaryRecordCount = 0;
	std::uint32_t PipelineLayoutRecordCount = 0;
	std::uint32_t ReflectionRecordCount = 0;
	CookedShaderPackageLoadKind Kind = CookedShaderPackageLoadKind::Disk;
};

class SPARKLE_RHI_API CookedShaderPackageCache final
{
  public:
	std::uint64_t GetGeneration() const noexcept { return m_generation; }
	const CookedShaderPackageLoadReport& GetLastLoadReport() const noexcept { return m_lastLoadReport; }
	void Clear() noexcept;
	void ReplaceWith(CookedShaderPackageCache&& replacement) noexcept;

	const LoadedShaderPackage& LoadPackage(
	    const ShaderPackageDefinition& definition,
	    const PassParameterLayout& expectedBindingLayout,
	    CookedShaderBinaryFormat runtimeBinaryFormat);

  private:
	static LoadedShaderPackage LoadPackageFromFile(const std::filesystem::path& path);
	static void ValidatePackage(
	    const LoadedShaderPackage& package,
	    const ShaderPackageDefinition& definition,
	    const PassParameterLayout& expectedBindingLayout,
	    CookedShaderBinaryFormat runtimeBinaryFormat);

	std::unordered_map<std::uint64_t, std::unique_ptr<LoadedShaderPackage>> m_packages;
	CookedShaderPackageLoadReport m_lastLoadReport;
	std::uint64_t m_generation = 1;
};
