#include "PCH.h"

#include "Contracts/ShaderContractCatalogBuilder.h"

#include "Core/Public/Diagnostics/Error.h"
#include "RHI/Public/Shaders/ShaderMap.h"
#include "ShaderContractCatalog.h"
#include "Shaders/Authoring/GlobalShader.h"
#include "Shaders/ShaderParameterLayoutBuilder.h"

#include <algorithm>
#include <format>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

ShaderContractCatalog ShaderContractCatalogBuilder::Build(ShaderContractSelectionKind selectionKind, std::string_view requestedId)
{
	ShaderContractCatalog catalog;
	std::unordered_map<ShaderTypeId, std::string> shaderTypeNames;
	for (const ShaderRegistrationDesc& registration : GlobalShaderRegistry::GetRegistrations())
	{
		const auto [existing, inserted] = shaderTypeNames.emplace(registration.TypeId, registration.ShaderName);
		if (!inserted && existing->second != registration.ShaderName)
		{
			throw Diagnostics::Error(
			    std::format("Shader type id collision between '{}' and '{}'.", existing->second, registration.ShaderName));
		}
		if (selectionKind != ShaderContractSelectionKind::All && registration.ShaderName != requestedId)
		{
			continue;
		}

		ShaderContract shader;
		shader.shaderTypeId = registration.TypeId;
		shader.shaderName = registration.ShaderName;
		shader.sourcePath = registration.SourcePath;
		shader.entryPoint = registration.EntryPoint;
		shader.stage = registration.Stage;
		shader.features = registration.Features;
		shader.rayTracing = registration.RayTracing;
		if (registration.BuildParameterStructDescriptor != nullptr)
		{
			shader.parameterStruct = registration.BuildParameterStructDescriptor();
			shader.parameterLayout = BuildShaderParameterLayout(registration);
			shader.hasParameterStruct = true;
		}
		catalog.push_back(std::move(shader));
	}

	std::ranges::sort(
	    catalog,
	    [](const ShaderContract& left, const ShaderContract& right) { return left.shaderTypeId < right.shaderTypeId; });
	if (selectionKind != ShaderContractSelectionKind::All && catalog.empty())
	{
		throw Diagnostics::Error(std::format("Unknown registered shader '{}'.", requestedId));
	}
	return catalog;
}
