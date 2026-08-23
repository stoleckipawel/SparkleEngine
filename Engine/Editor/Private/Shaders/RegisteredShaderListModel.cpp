#include "PCH.h"
#include "Core/Public/FileSystemUtils.h"

#include "Shaders/RegisteredShaderListModel.h"

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

		RegisteredShaderRow row;
		row.ShaderId = std::string(shader.ShaderName);
		row.ShaderTypeId = shader.TypeId;
		row.SourcePath = std::string(shader.SourcePath);
		row.EntryPoint = std::string(shader.EntryPoint);
		row.Stage = GetShaderStagePrefix(shader.Stage);
		row.ParameterCount = parameters.Fields.size();
		row.RuntimeGeneration = generation;
		row.ArtifactDirectory = FindDebugArtifactDirectoryFor(row.ShaderId);
		row.ArtifactAvailable = !row.ArtifactDirectory.empty();
		m_rows.push_back(std::move(row));
	}
}

std::filesystem::path RegisteredShaderListModel::FindDebugArtifactDirectoryFor(std::string_view shaderId)
{
	const std::filesystem::path root = Filesystem::GetShaderSymbolsOutputPath();
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
		if (Strings::ContainsIgnoreCase(directoryName, shaderId) &&
		    std::filesystem::exists(it->path() / "compile-request.json", errorCode) && !errorCode)
		{
			return it->path();
		}
		errorCode.clear();
	}
	return {};
}
