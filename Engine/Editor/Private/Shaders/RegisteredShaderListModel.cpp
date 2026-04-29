#include "PCH.h"

#include "Shaders/RegisteredShaderListModel.h"

#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Strings/StringUtils.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"
#include "RHI/Public/Shaders/ShaderStage.h"

#include <filesystem>
#include <system_error>
#include <utility>

void RegisteredShaderListModel::SetGenerationProvider(GenerationProvider provider)
{
	m_generationProvider = std::move(provider);
}

void RegisteredShaderListModel::Refresh()
{
	m_rows.clear();
	const std::uint64_t generation = m_generationProvider ? m_generationProvider() : 0;
	for (const ShaderRegistrationDesc& shader : GlobalShaderRegistry::GetRegistrations())
	{
		const ShaderParameterStructDescriptor parameters =
		    shader.BuildParameterStructDescriptor != nullptr ? shader.BuildParameterStructDescriptor() : ShaderParameterStructDescriptor{};
		const ShaderPermutationDomainDescriptor permutations =
		    shader.BuildPermutationDomainDescriptor != nullptr ? shader.BuildPermutationDomainDescriptor() : ShaderPermutationDomainDescriptor{};

		RegisteredShaderRow row;
		row.ShaderId = std::string(shader.ShaderName);
		row.PackageId = shader.PackageName.empty() ? row.ShaderId : std::string(shader.PackageName);
		row.SourcePath = std::string(shader.SourcePath);
		row.EntryPoint = std::string(shader.EntryPoint);
		row.Stage = GetShaderStagePrefix(shader.Stage);
		row.BindingLayoutId = shader.BindingLayoutId.empty() ? "Empty" : std::string(shader.BindingLayoutId);
		row.ParameterCount = parameters.Fields.size();
		row.PermutationDimensionCount = permutations.Dimensions.size();
		row.RuntimeGeneration = generation;
		row.ArtifactDirectory = FindDebugArtifactDirectoryFor(row.ShaderId, row.PackageId);
		row.ArtifactAvailable = !row.ArtifactDirectory.empty();
		row.LastStatus = m_lastStatus;
		m_rows.push_back(std::move(row));
	}
}

void RegisteredShaderListModel::SetLastStatus(std::string status)
{
	m_lastStatus = std::move(status);
	for (RegisteredShaderRow& row : m_rows)
	{
		row.LastStatus = m_lastStatus;
	}
}

std::filesystem::path RegisteredShaderListModel::FindDebugArtifactDirectoryFor(std::string_view shaderId, std::string_view packageId)
{
	const std::filesystem::path root = Paths::ShaderCacheRoot();
	std::error_code errorCode;
	if (!std::filesystem::exists(root, errorCode) || errorCode)
	{
		return {};
	}

	for (std::filesystem::recursive_directory_iterator it(root, errorCode), end; it != end && !errorCode; it.increment(errorCode))
	{
		if (!it->is_directory(errorCode) || errorCode)
		{
			errorCode.clear();
			continue;
		}

		const std::string directoryName = it->path().filename().generic_string();
		if ((Strings::ContainsIgnoreCase(directoryName, shaderId) || Strings::ContainsIgnoreCase(directoryName, packageId)) &&
		    std::filesystem::exists(it->path() / "compile-request.json", errorCode) && !errorCode)
		{
			return it->path();
		}
		errorCode.clear();
	}
	return {};
}
