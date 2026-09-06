#include "PCH.h"

#include "Cooking/ShaderCookPlanner.h"

#include "Compiler/ShaderSourceMountTable.h"
#include "Cooking/Dependencies/ShaderDependencyManifest.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/FileSystemUtils.h"

#include <algorithm>
#include <format>
#include <unordered_set>

std::vector<ShaderCookDesc> ShaderCookPlanner::BuildShaders(
    const ShaderCookSettings& settings,
    const ShaderDependencyManifest& dependencyManifest,
    const ShaderContractCatalog& catalog)
{
	const std::uint32_t selectionCount =
	    static_cast<std::uint32_t>(!settings.shaderId.empty()) + static_cast<std::uint32_t>(!settings.changedVirtualPaths.empty());
	if (selectionCount > 1)
	{
		throw Diagnostics::Error("Use one shader cook selection: shader id or changed virtual paths.");
	}

	std::unordered_set<ShaderTypeId> affectedShaderTypes;
	if (!settings.changedVirtualPaths.empty())
	{
		const ShaderSourceMountTable sourceMounts(
		    Filesystem::GetShaderPath(PathRoot::Engine),
		    Filesystem::GetShaderPath(PathRoot::Project));
		std::vector<std::string> canonicalChangedPaths;
		canonicalChangedPaths.reserve(settings.changedVirtualPaths.size());
		for (const std::string& changedPath : settings.changedVirtualPaths)
		{
			canonicalChangedPaths.push_back(sourceMounts.CanonicalizeVirtualPath(changedPath));
		}
		std::ranges::sort(canonicalChangedPaths);
		canonicalChangedPaths.erase(std::unique(canonicalChangedPaths.begin(), canonicalChangedPaths.end()), canonicalChangedPaths.end());
		affectedShaderTypes = dependencyManifest.SelectAffectedShaderTypes(canonicalChangedPaths);
	}

	std::vector<ShaderCookDesc> shaders;
	for (const ShaderContract& contract : catalog)
	{
		if (!settings.shaderId.empty() && contract.shaderName != settings.shaderId)
		{
			continue;
		}
		if (!settings.changedVirtualPaths.empty() && !affectedShaderTypes.contains(contract.shaderTypeId))
		{
			continue;
		}
		shaders.push_back(
		    ShaderCookDesc{
		        .shaderTypeId = contract.shaderTypeId,
		        .shaderTypeName = contract.shaderName,
		        .stage = contract.stage,
		        .sourcePath = contract.sourcePath,
		        .entryPoint = contract.entryPoint,
		        .features = contract.features,
		        .rayTracing = contract.rayTracing,
		        .parameterLayout = contract.parameterLayout,
		        .parameterStruct = contract.hasParameterStruct ? std::optional(contract.parameterStruct) : std::nullopt});
	}
	if (!settings.shaderId.empty() && shaders.empty())
	{
		throw Diagnostics::Error(std::format("Unknown registered shader '{}'.", settings.shaderId));
	}
	return shaders;
}
