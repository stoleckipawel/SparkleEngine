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

ShaderCookPipelinePlan ShaderCookPlanBuilder::Build(const ShaderCookSettings& settings, ShaderBackendPool& backendPool)
{
	ShaderCookPipelinePlan plan;
	const std::filesystem::path dependencyPath = ShaderDependencyManifest::GetPath(Filesystem::GetCookedShaderRootPath());
	const bool fullCatalog = settings.shaderId.empty() && settings.changedVirtualPaths.empty();
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

	plan.shaders = ShaderCookPlanner::BuildShaders(settings, plan.dependencyManifest);
	plan.shaderOutputs.resize(plan.shaders.size());
	for (std::size_t shaderIndex = 0; shaderIndex < plan.shaders.size(); ++shaderIndex)
	{
		plan.shaderOutputs[shaderIndex].reserve(settings.targets.size());
		for (const ShaderTarget target : settings.targets)
		{
			ShaderCompileJobBuilder::BuildAndAdd(settings, shaderIndex, target, backendPool, plan);
		}
	}
	BuildDependencyManifest(settings, plan);
	return plan;
}

void ShaderCookPlanBuilder::BuildDependencyManifest(const ShaderCookSettings& settings, ShaderCookPipelinePlan& plan)
{
	const bool fullCatalog = settings.shaderId.empty() && settings.changedVirtualPaths.empty();
	std::unordered_map<ShaderTypeId, ShaderDependencyRecord> records;
	for (const ShaderCompileConsumer& consumer : plan.consumers)
	{
		const ShaderCompileJob& job = plan.jobs[consumer.JobIndex];
		auto [record, inserted] = records.try_emplace(
		    job.Request.ShaderType,
		    ShaderDependencyRecord{
		        .ShaderType = job.Request.ShaderType,
		        .ShaderTypeName = job.Request.ShaderTypeName,
		        .VirtualSourcePath = job.Request.VirtualSourcePath});
		if (!inserted
		    && (record->second.ShaderTypeName != job.Request.ShaderTypeName
		        || record->second.VirtualSourcePath != job.Request.VirtualSourcePath))
		{
			throw Diagnostics::Error(std::format("Shader type '{}' produced conflicting dependency identities.", job.Request.ShaderTypeName));
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
