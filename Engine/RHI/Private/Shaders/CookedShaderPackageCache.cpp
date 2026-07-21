#include "PCH.h"

#include "Shaders/CookedShaderPackageCache.h"

#include "Core/Public/Files/BinarySpanReader.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "ShaderParameters/PassParameterLayout.h"
#include "Shaders/CookedShaderPackageContract.h"
#include "Shaders/ShaderRayTracingMetadataValidation.h"

#include <array>
#include <chrono>
#include <format>
#include <string>
#include <vector>

namespace
{
	using PackageLoadClock = std::chrono::steady_clock;

	std::uint64_t ToElapsedMicroseconds(PackageLoadClock::time_point start) noexcept
	{
		const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(PackageLoadClock::now() - start);
		return static_cast<std::uint64_t>(elapsed.count());
	}

	CookedShaderPackageLoadReport MakeLoadReport(
	    std::uint64_t packageKey,
	    std::filesystem::path packagePath,
	    std::uint64_t generation,
	    bool wasCacheHit,
	    bool wasReload,
	    bool succeeded,
	    std::uint64_t elapsedMicroseconds,
	    const LoadedShaderPackage* package)
	{
		CookedShaderPackageLoadReport report{};
		report.PackageKey = packageKey;
		report.PackagePath = std::move(packagePath);
		report.CacheGeneration = generation;
		report.ElapsedMicroseconds = elapsedMicroseconds;
		report.WasCacheHit = wasCacheHit;
		report.WasReload = wasReload;
		report.Succeeded = succeeded;
		if (package != nullptr)
		{
			const CookedShaderPackageHeader& header = package->GetHeader();
			report.BinaryRecordCount = header.BinaryRecordCount;
			report.PipelineLayoutRecordCount = header.PipelineLayoutRecordCount;
			report.ReflectionRecordCount = header.ReflectionRecordCount;
		}
		return report;
	}

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
	const PackageLoadClock::time_point loadStart = PackageLoadClock::now();
	if (!definition.IsValid())
	{
		m_lastLoadReport = MakeLoadReport(0, {}, m_generation, false, false, false, ToElapsedMicroseconds(loadStart), nullptr);
		outErrorMessage = "Shader package definition is invalid.";
		return false;
	}

	const std::uint64_t packageKey = BuildShaderPackageKey(definition.PackageId);
	const std::filesystem::path packagePath = Paths::CookedShaderPackage(packageKey);
	if (auto it = m_packages.find(packageKey); it != m_packages.end())
	{
		if (!ValidatePackage(*it->second, definition, expectedBindingLayout, requiredBinaryFormat, outErrorMessage))
		{
			m_lastLoadReport =
			    MakeLoadReport(packageKey, packagePath, m_generation, true, false, false, ToElapsedMicroseconds(loadStart), it->second.get());
			return false;
		}

		outPackage = it->second.get();
		m_lastLoadReport =
		    MakeLoadReport(packageKey, packagePath, m_generation, true, false, true, ToElapsedMicroseconds(loadStart), outPackage);
		outErrorMessage.clear();
		return true;
	}

	auto loadedPackage = std::make_unique<LoadedShaderPackage>();
	if (!LoadPackageFromFile(packagePath, *loadedPackage, outErrorMessage))
	{
		m_lastLoadReport = MakeLoadReport(packageKey, packagePath, m_generation, false, false, false, ToElapsedMicroseconds(loadStart), nullptr);
		return false;
	}

	if (!ValidatePackage(*loadedPackage, definition, expectedBindingLayout, requiredBinaryFormat, outErrorMessage))
	{
		m_lastLoadReport =
		    MakeLoadReport(packageKey, packagePath, m_generation, false, false, false, ToElapsedMicroseconds(loadStart), loadedPackage.get());
		return false;
	}

	LoadedShaderPackage* cachedPackage = loadedPackage.get();
	m_packages.emplace(packageKey, std::move(loadedPackage));
	outPackage = cachedPackage;
	m_lastLoadReport =
	    MakeLoadReport(packageKey, packagePath, m_generation, false, false, true, ToElapsedMicroseconds(loadStart), outPackage);
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
	const PackageLoadClock::time_point loadStart = PackageLoadClock::now();
	if (!definition.IsValid())
	{
		m_lastLoadReport = MakeLoadReport(0, {}, m_generation, false, true, false, ToElapsedMicroseconds(loadStart), nullptr);
		outErrorMessage = "Shader package definition is invalid.";
		return false;
	}

	const std::uint64_t packageKey = BuildShaderPackageKey(definition.PackageId);
	auto loadedPackage = std::make_unique<LoadedShaderPackage>();
	const std::filesystem::path packagePath = Paths::CookedShaderPackage(packageKey);
	if (!LoadPackageFromFile(packagePath, *loadedPackage, outErrorMessage))
	{
		m_lastLoadReport = MakeLoadReport(packageKey, packagePath, m_generation, false, true, false, ToElapsedMicroseconds(loadStart), nullptr);
		return false;
	}

	if (!ValidatePackage(*loadedPackage, definition, expectedBindingLayout, requiredBinaryFormat, outErrorMessage))
	{
		m_lastLoadReport =
		    MakeLoadReport(packageKey, packagePath, m_generation, false, true, false, ToElapsedMicroseconds(loadStart), loadedPackage.get());
		return false;
	}

	LoadedShaderPackage* cachedPackage = loadedPackage.get();
	m_packages[packageKey] = std::move(loadedPackage);
	++m_generation;
	outPackage = cachedPackage;
	m_lastLoadReport =
	    MakeLoadReport(packageKey, packagePath, m_generation, false, true, true, ToElapsedMicroseconds(loadStart), outPackage);
	outErrorMessage.clear();
	return true;
}
