#include "PCH.h"

#include "Shaders/CookedShaderPackageCache.h"

#include "Core/Public/Diagnostics/Error.h"
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

class CookedShaderPackageCacheImplementation final
{
  public:
	using PackageLoadClock = std::chrono::steady_clock;

	static std::uint64_t ToElapsedMicroseconds(PackageLoadClock::time_point start) noexcept
	{
		const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(PackageLoadClock::now() - start);
		return static_cast<std::uint64_t>(elapsed.count());
	}

	static CookedShaderPackageLoadReport MakeLoadReport(
	    std::uint64_t packageKey,
	    std::filesystem::path packagePath,
	    std::uint64_t generation,
	    CookedShaderPackageLoadKind kind,
	    std::uint64_t elapsedMicroseconds,
	    const LoadedShaderPackage* package)
	{
		CookedShaderPackageLoadReport report{};
		report.PackageKey = packageKey;
		report.PackagePath = std::move(packagePath);
		report.CacheGeneration = generation;
		report.ElapsedMicroseconds = elapsedMicroseconds;
		report.Kind = kind;
		if (package != nullptr)
		{
			const CookedShaderPackageHeader& header = package->GetHeader();
			report.BinaryRecordCount = header.BinaryRecordCount;
			report.PipelineLayoutRecordCount = header.PipelineLayoutRecordCount;
			report.ReflectionRecordCount = header.ReflectionRecordCount;
		}
		return report;
	}

};

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

const LoadedShaderPackage& CookedShaderPackageCache::LoadPackage(
    const ShaderPackageDefinition& definition,
    const PassParameterLayout& expectedBindingLayout,
    CookedShaderBinaryFormat runtimeBinaryFormat)
{
	const CookedShaderPackageCacheImplementation::PackageLoadClock::time_point loadStart = CookedShaderPackageCacheImplementation::PackageLoadClock::now();
	if (!definition.IsValid())
	{
		m_lastLoadReport = CookedShaderPackageCacheImplementation::MakeLoadReport(
		    0,
		    {},
		    m_generation,
		    CookedShaderPackageLoadKind::Disk,
		    CookedShaderPackageCacheImplementation::ToElapsedMicroseconds(loadStart),
		    nullptr);
		throw Diagnostics::Error("Shader package definition is invalid.");
	}

	const std::uint64_t packageKey = BuildShaderPackageKey(definition.PackageId);
	const std::filesystem::path packagePath = Paths::CookedShaderPackage(packageKey);
	if (auto it = m_packages.find(packageKey); it != m_packages.end())
	{
		try
		{
			ValidatePackage(*it->second, definition, expectedBindingLayout, runtimeBinaryFormat);
		}
		catch (const Diagnostics::Error&)
		{
			m_lastLoadReport = CookedShaderPackageCacheImplementation::MakeLoadReport(
			    packageKey,
			    packagePath,
			    m_generation,
			    CookedShaderPackageLoadKind::CacheHit,
			    CookedShaderPackageCacheImplementation::ToElapsedMicroseconds(loadStart),
			    it->second.get());
			throw;
		}
		m_lastLoadReport =
		    CookedShaderPackageCacheImplementation::MakeLoadReport(
		        packageKey,
		        packagePath,
		        m_generation,
		        CookedShaderPackageLoadKind::CacheHit,
		        CookedShaderPackageCacheImplementation::ToElapsedMicroseconds(loadStart),
		        it->second.get());
		return *it->second;
	}

	std::unique_ptr<LoadedShaderPackage> loadedPackage;
	try
	{
		loadedPackage = std::make_unique<LoadedShaderPackage>(LoadPackageFromFile(packagePath));
		ValidatePackage(*loadedPackage, definition, expectedBindingLayout, runtimeBinaryFormat);
	}
	catch (const Diagnostics::Error&)
	{
		m_lastLoadReport = CookedShaderPackageCacheImplementation::MakeLoadReport(
		    packageKey,
		    packagePath,
		    m_generation,
		    CookedShaderPackageLoadKind::Disk,
		    CookedShaderPackageCacheImplementation::ToElapsedMicroseconds(loadStart),
		    loadedPackage.get());
		throw;
	}

	LoadedShaderPackage* cachedPackage = loadedPackage.get();
	m_packages.emplace(packageKey, std::move(loadedPackage));
	m_lastLoadReport =
	    CookedShaderPackageCacheImplementation::MakeLoadReport(
	        packageKey,
	        packagePath,
	        m_generation,
	        CookedShaderPackageLoadKind::Disk,
	        CookedShaderPackageCacheImplementation::ToElapsedMicroseconds(loadStart),
	        cachedPackage);
	return *cachedPackage;
}
