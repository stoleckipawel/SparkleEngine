#include "PCH.h"

#include "Cooking/ShaderCookPlanBuilder.h"

#include "Backend/ShaderBackendPool.h"
#include "Cooking/ShaderCompileJobBuilder.h"
#include "Cooking/ShaderCookPlanner.h"
#include "Cooking/ShaderCookSettings.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/FileSystemUtils.h"

#include <format>
#include <unordered_map>
#include <utility>

ShaderCookPipelinePlan ShaderCookPlanBuilder::Build(const ShaderPackageCookSettings& settings, ShaderBackendPool& backendPool)
{
	ShaderCookPipelinePlan plan;
	const std::filesystem::path dependencyPath = ShaderDependencyManifest::GetPath(Filesystem::GetCookedShaderRootPath());
	const bool fullCatalog = settings.packageId.empty() && settings.shaderId.empty() && settings.changedVirtualPaths.empty();
	if (!settings.changedVirtualPaths.empty())
	{
		plan.dependencyManifest = ShaderDependencyManifest::ReadRequired(dependencyPath);
	}
	else if (!fullCatalog)
	{
		if (std::optional<ShaderDependencyManifest> existing = ShaderDependencyManifest::ReadForUpdate(dependencyPath))
		{
			plan.dependencyManifest = std::move(*existing);
		}
	}
	plan.packages = ShaderCookPlanner::BuildPackages(settings, plan.dependencyManifest);

	plan.packageContexts.resize(plan.packages.size());
	for (std::size_t packageIndex = 0; packageIndex < plan.packages.size(); ++packageIndex)
	{
		AddPackageJobs(settings, packageIndex, backendPool, plan);
	}
	BuildDependencyManifest(settings, plan);

	return plan;
}

void ShaderCookPlanBuilder::AddPackageJobs(
    const ShaderPackageCookSettings& settings,
    std::size_t packageIndex,
    ShaderBackendPool& backendPool,
    ShaderCookPipelinePlan& plan)
{
	const ShaderCookPackageDesc& package = plan.packages[packageIndex];
	ShaderCookPackageContext& packageContext = plan.packageContexts[packageIndex];
	packageContext.reserve(package.stages.size() * settings.targets.size());
	for (std::size_t targetIndex = 0; targetIndex < settings.targets.size(); ++targetIndex)
	{
		for (std::size_t stageIndex = 0; stageIndex < package.stages.size(); ++stageIndex)
		{
			ShaderCompileJobBuilder::BuildAndAdd(settings, packageIndex, stageIndex, settings.targets[targetIndex], backendPool, plan);
		}
	}
}

void ShaderCookPlanBuilder::BuildDependencyManifest(const ShaderPackageCookSettings& settings, ShaderCookPipelinePlan& plan)
{
	const bool fullCatalog = settings.packageId.empty() && settings.shaderId.empty() && settings.changedVirtualPaths.empty();
	std::unordered_map<ShaderTypeId, ShaderDependencyRecord> records;
	for (const ShaderCompileConsumer& consumer : plan.consumers)
	{
		const ShaderCompileJob& job = plan.jobs[consumer.JobIndex];
		const std::string& publicationGroup = plan.packages[consumer.PackageIndex].packageId;
		auto [record, inserted] = records.try_emplace(
		    job.Request.ShaderType,
		    ShaderDependencyRecord{
		        .ShaderType = job.Request.ShaderType,
		        .ShaderTypeName = job.Request.ShaderTypeName,
		        .VirtualSourcePath = job.Request.VirtualSourcePath,
		        .PublicationGroup = publicationGroup});
		if (!inserted
		    && (record->second.ShaderTypeName != job.Request.ShaderTypeName
		        || record->second.VirtualSourcePath != job.Request.VirtualSourcePath
		        || record->second.PublicationGroup != publicationGroup))
		{
			throw Diagnostics::Error(
			    std::format("Shader type '{}' produced conflicting dependency identities.", job.Request.ShaderTypeName));
		}
		record->second.VirtualDependencies.insert(
		    record->second.VirtualDependencies.end(),
		    job.VirtualDependencies.begin(),
		    job.VirtualDependencies.end());
	}
	for (auto& [shaderType, record] : records)
	{
		(void) shaderType;
		plan.dependencyManifest.Replace(std::move(record));
	}
	plan.dependencyManifest.SetCompleteCatalog(fullCatalog || plan.dependencyManifest.IsCompleteCatalog());
	plan.dependencyManifest.SortAndValidate();
}
