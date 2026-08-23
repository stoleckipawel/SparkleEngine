#include "PCH.h"

#include "Cooking/CookedShaderPackageEmitter.h"

#include "Cooking/CookedPackageWriter.h"
#include "Cooking/CookedRegistryWriter.h"
#include "Cooking/Dependencies/ShaderDependencyManifest.h"
#include "Cooking/ShaderRecookSignal.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "RHI/Public/Shaders/CookedShaderPackageIdentity.h"

#include <utility>
#include <vector>

class CookedShaderPublication final
{
public:
	CookedShaderPublication(const ShaderCookPipelinePlan& plan, const std::filesystem::path& outputDirectory) noexcept;

	std::vector<CookedShaderPackageOutput> Publish();

private:
	void StagePackages();
	void StagePackage(std::size_t packageIndex);
	void StageRegistry();
	void StageDependencies();
	void StageRecookSignal();
	void PublishFiles();
	void CleanupStagedFiles() const;

	const ShaderCookPipelinePlan& m_plan;
	const std::filesystem::path& m_outputDirectory;
	std::vector<CookedShaderPackageOutput> m_packages;
	std::vector<Files::FilePublication> m_files;
	std::filesystem::path m_registryPath;
	std::filesystem::path m_stagedRegistryPath;
	std::filesystem::path m_dependencyPath;
	std::filesystem::path m_stagedDependencyPath;
};

CookedShaderPublication::CookedShaderPublication(const ShaderCookPipelinePlan& plan, const std::filesystem::path& outputDirectory) noexcept
    :
    m_plan(plan),
    m_outputDirectory(outputDirectory)
{
}

std::vector<CookedShaderPackageOutput> CookedShaderPublication::Publish()
{
	m_packages.reserve(m_plan.packages.size());
	m_files.reserve(m_plan.packages.size() + 3);

	try
	{
		StagePackages();
		StageRegistry();
		StageDependencies();
		StageRecookSignal();
		PublishFiles();
	}
	catch (...)
	{
		CleanupStagedFiles();
		throw;
	}

	return std::move(m_packages);
}

void CookedShaderPublication::StagePackages()
{
	for (std::size_t packageIndex = 0; packageIndex < m_plan.packages.size(); ++packageIndex)
	{
		StagePackage(packageIndex);
	}
}

void CookedShaderPublication::StagePackage(std::size_t packageIndex)
{
	const ShaderCookPackageDesc& package = m_plan.packages[packageIndex];
	const ShaderCookPackageContext& packageContext = m_plan.packageContexts[packageIndex];
	const std::filesystem::path publishedPath = Paths::CookedShaderPackage(BuildShaderPackageKey(package.packageId));
	const std::filesystem::path stagedPath = Files::BuildTemporaryPath(publishedPath, ".cook-generation");
	Files::CleanupTemporaryFile(stagedPath);

	try
	{
		m_packages.push_back(CookedPackageWriter::Write(package, packageContext, stagedPath, publishedPath));
	}
	catch (const Diagnostics::Error& error)
	{
		Files::CleanupTemporaryFile(stagedPath);
		throw Diagnostics::Error("Failed to emit cooked shader package '" + package.packageId + "' - " + error.what());
	}

	m_files.push_back({stagedPath, publishedPath});
}

void CookedShaderPublication::StageRegistry()
{
	m_registryPath = Filesystem::GetCookedShaderRegistryPath();
	m_stagedRegistryPath = Files::BuildTemporaryPath(m_registryPath, ".cook-generation");
	Files::CleanupTemporaryFile(m_stagedRegistryPath);

	try
	{
		CookedRegistryWriter::Write(m_packages, m_stagedRegistryPath);
	}
	catch (...)
	{
		Files::CleanupTemporaryFile(m_stagedRegistryPath);
		throw;
	}

	m_files.push_back({m_stagedRegistryPath, m_registryPath});
}

void CookedShaderPublication::StageDependencies()
{
	m_dependencyPath = ShaderDependencyManifest::GetPath(m_outputDirectory);
	m_stagedDependencyPath = Files::BuildTemporaryPath(m_dependencyPath, ".cook-generation");
	Files::CleanupTemporaryFile(m_stagedDependencyPath);
	ShaderDependencyManifest::Write(m_plan.dependencyManifest, m_stagedDependencyPath);
	m_files.push_back({m_stagedDependencyPath, m_dependencyPath});
}

void CookedShaderPublication::StageRecookSignal()
{
	const std::filesystem::path publishedSignalPath = Paths::ShaderRecookSignal(m_outputDirectory);
	const std::filesystem::path stagedSignalPath = Files::BuildTemporaryPath(publishedSignalPath, ".cook-generation");
	Files::CleanupTemporaryFile(stagedSignalPath);

	try
	{
		ShaderRecookSignal::Write(m_stagedRegistryPath, m_registryPath, stagedSignalPath);
	}
	catch (...)
	{
		Files::CleanupTemporaryFile(stagedSignalPath);
		throw;
	}

	m_files.push_back({stagedSignalPath, publishedSignalPath});
}

void CookedShaderPublication::PublishFiles()
{
	std::string publicationError;
	if (!Files::TryPublishFileSet(m_files, publicationError))
	{
		throw Diagnostics::Error(std::move(publicationError));
	}
}

void CookedShaderPublication::CleanupStagedFiles() const
{
	for (const Files::FilePublication& file : m_files)
	{
		Files::CleanupTemporaryFile(file.StagedPath);
	}
}

std::vector<CookedShaderPackageOutput> CookedShaderPackageEmitter::Emit(
    const ShaderCookPipelinePlan& plan,
    const std::filesystem::path& outputDirectory)
{
	return CookedShaderPublication(plan, outputDirectory).Publish();
}
