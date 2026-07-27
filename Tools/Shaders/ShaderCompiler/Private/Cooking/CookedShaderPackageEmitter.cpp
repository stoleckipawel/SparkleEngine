#include "PCH.h"

#include "Cooking/CookedShaderPackageEmitter.h"

#include "Cooking/CookedPackageWriter.h"
#include "Cooking/CookedRegistryWriter.h"
#include "Cooking/ShaderCookResult.h"
#include "Cooking/ShaderRecookSignal.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "RHI/Public/Shaders/CookedShaderPackageIdentity.h"

#include <utility>
#include <vector>

class CookedShaderPublication final
{
  public:
	CookedShaderPublication(
	    const ShaderCookPipelinePlan& plan,
	    const std::filesystem::path& cacheDirectory,
	    ShaderPackageCookResult& result,
	    std::string& errorMessage) noexcept;

	bool Publish();

  private:
	bool StagePackages();
	bool StagePackage(std::size_t packageIndex);
	bool StageRegistry();
	bool StageRecookSignal();
	bool PublishFiles();
	void CleanupStagedFiles() const;

	const ShaderCookPipelinePlan& m_plan;
	const std::filesystem::path& m_cacheDirectory;
	ShaderPackageCookResult& m_result;
	std::string& m_errorMessage;
	std::vector<Files::FilePublication> m_files;
	std::filesystem::path m_registryPath;
	std::filesystem::path m_stagedRegistryPath;
};

CookedShaderPublication::CookedShaderPublication(
    const ShaderCookPipelinePlan& plan,
    const std::filesystem::path& cacheDirectory,
    ShaderPackageCookResult& result,
    std::string& errorMessage) noexcept :
    m_plan(plan),
    m_cacheDirectory(cacheDirectory),
    m_result(result),
    m_errorMessage(errorMessage)
{
}

bool CookedShaderPublication::Publish()
{
	m_result.packages.clear();
	m_result.packages.reserve(m_plan.packages.size());
	m_files.reserve(m_plan.packages.size() + 2);

	if (!StagePackages() || !StageRegistry() || !StageRecookSignal() || !PublishFiles())
	{
		CleanupStagedFiles();
		m_result.packages.clear();
		return false;
	}

	m_errorMessage.clear();
	return true;
}

bool CookedShaderPublication::StagePackages()
{
	for (std::size_t packageIndex = 0; packageIndex < m_plan.packages.size(); ++packageIndex)
	{
		if (!StagePackage(packageIndex))
		{
			return false;
		}
	}

	return true;
}

bool CookedShaderPublication::StagePackage(std::size_t packageIndex)
{
	const ShaderCookPackageDesc& package = m_plan.packages[packageIndex];
	const ShaderCookPackageContext& packageContext = m_plan.packageContexts[packageIndex];
	const std::filesystem::path publishedPath =
	    Paths::CookedShaderPackage(BuildShaderPackageKey(package.packageId));
	const std::filesystem::path stagedPath =
	    Files::BuildTemporaryPath(publishedPath, ".cook-generation");
	Files::CleanupTemporaryFile(stagedPath);

	CookedShaderPackageOutput packageOutput;
	if (!CookedPackageWriter::Write(
	        package,
	        packageContext.compiledStages,
	        stagedPath,
	        publishedPath,
	        packageOutput,
	        m_errorMessage))
	{
		m_errorMessage = "Failed to emit cooked shader package '" + package.packageId + "' - " + m_errorMessage;
		Files::CleanupTemporaryFile(stagedPath);
		return false;
	}

	m_files.push_back({stagedPath, publishedPath});
	m_result.packages.push_back(std::move(packageOutput));
	return true;
}

bool CookedShaderPublication::StageRegistry()
{
	m_registryPath = Filesystem::GetCookedShaderRegistryPath();
	m_stagedRegistryPath = Files::BuildTemporaryPath(m_registryPath, ".cook-generation");
	Files::CleanupTemporaryFile(m_stagedRegistryPath);

	if (!CookedRegistryWriter::Write(m_result.packages, m_stagedRegistryPath, m_errorMessage))
	{
		Files::CleanupTemporaryFile(m_stagedRegistryPath);
		return false;
	}

	m_files.push_back({m_stagedRegistryPath, m_registryPath});
	return true;
}

bool CookedShaderPublication::StageRecookSignal()
{
	ShaderRecookSignalResult signalResult;
	const std::filesystem::path publishedSignalPath = Paths::ShaderRecookSignal(m_cacheDirectory);
	const std::filesystem::path stagedSignalPath =
	    Files::BuildTemporaryPath(publishedSignalPath, ".cook-generation");
	Files::CleanupTemporaryFile(stagedSignalPath);

	if (!ShaderRecookSignal::Write(
	        m_stagedRegistryPath,
	        m_registryPath,
	        stagedSignalPath,
	        publishedSignalPath,
	        signalResult,
	        m_errorMessage))
	{
		Files::CleanupTemporaryFile(stagedSignalPath);
		return false;
	}

	m_files.push_back({stagedSignalPath, publishedSignalPath});
	return true;
}

bool CookedShaderPublication::PublishFiles()
{
	if (!Files::TryPublishFileSet(m_files, m_errorMessage))
	{
		return false;
	}

	return true;
}

void CookedShaderPublication::CleanupStagedFiles() const
{
	for (const Files::FilePublication& file : m_files)
	{
		Files::CleanupTemporaryFile(file.StagedPath);
	}
}

bool CookedShaderPackageEmitter::Emit(
    const ShaderCookPipelinePlan& plan,
    const std::filesystem::path& cacheDirectory,
    ShaderPackageCookResult& result,
    std::string& outErrorMessage)
{
	return CookedShaderPublication(plan, cacheDirectory, result, outErrorMessage).Publish();
}
