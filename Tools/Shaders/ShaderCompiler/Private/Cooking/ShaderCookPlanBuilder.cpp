#include "PCH.h"

#include "Cooking/ShaderCookPlanBuilder.h"

#include "Backend/ShaderBackendPool.h"
#include "Contracts/ShaderContractCatalogBuilder.h"
#include "Contracts/ShaderContractValidator.h"
#include "Cooking/ShaderCompileJobBuilder.h"
#include "Cooking/ShaderCookPlanner.h"
#include "Cooking/ShaderCookSettings.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/FileSystemUtils.h"

#include <algorithm>
#include <format>
#include <unordered_map>

ShaderCookPipelinePlan ShaderCookPlanBuilder::Build(const ShaderCookSettings& settings, ShaderBackendPool& backendPool)
{
	ShaderCookPipelinePlan plan;
	if (settings.targets.empty())
	{
		throw Diagnostics::Error("Shader cooking requires at least one target.");
	}
	for (std::size_t targetIndex = 0; targetIndex < settings.targets.size(); ++targetIndex)
	{
		if (!IsShaderTarget(settings.targets[targetIndex])
		    || std::find(settings.targets.begin(), settings.targets.begin() + targetIndex, settings.targets[targetIndex])
		        != settings.targets.begin() + targetIndex)
		{
			throw Diagnostics::Error("Shader cooking received an invalid or duplicate target.");
		}
	}
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

	const ShaderContractCatalog catalog = ShaderContractCatalogBuilder::Build(ShaderContractSelectionKind::All, {});
	const std::vector<ShaderContractVerificationFailure> contractFailures = ShaderContractValidator::Validate(catalog);
	if (catalog.empty())
	{
		throw Diagnostics::Error("Shader catalog is empty.");
	}
	if (!contractFailures.empty())
	{
		throw Diagnostics::Error("Shader contract invalid " + ShaderContractValidator::FormatFailure(contractFailures.front()));
	}
	plan.registeredShaderTypes.reserve(catalog.size());
	for (const ShaderContract& shader : catalog)
	{
		plan.registeredShaderTypes.push_back(shader.shaderTypeId);
	}
	plan.shaders = ShaderCookPlanner::BuildShaders(settings, plan.dependencyManifest, catalog);
	plan.shaderOutputs.resize(plan.shaders.size());
	for (std::size_t shaderIndex = 0; shaderIndex < plan.shaders.size(); ++shaderIndex)
	{
		plan.shaderOutputs[shaderIndex].reserve(settings.targets.size());
		for (const ShaderTarget target : settings.targets)
		{
			ShaderCompileJobBuilder::BuildAndAdd(settings, shaderIndex, target, backendPool, plan);
		}
	}
	BuildDependencyManifest(plan);
	return plan;
}

void ShaderCookPlanBuilder::BuildDependencyManifest(ShaderCookPipelinePlan& plan)
{
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
	plan.removedShaderTypeCount = plan.dependencyManifest.RemoveUnregisteredShaderTypes(plan.registeredShaderTypes);
	plan.dependencyManifest.SortAndValidate();
	if (!plan.dependencyManifest.MatchesShaderTypes(plan.registeredShaderTypes))
	{
		throw Diagnostics::Error(
		    "Shader dependency metadata does not cover the current registered catalog. Run RecompileShaders Global to rebuild all "
		    "shaders.");
	}
}
