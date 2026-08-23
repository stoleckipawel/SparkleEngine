#include "PCH.h"

#include "Cooking/Dependencies/ShaderDependencyManifest.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Files/FileUtils.h"

#include <algorithm>
#include <format>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string_view>
#include <unordered_map>

static constexpr std::string_view kShaderDependencyManifestHeader = "SparkleShaderDependencies";

std::filesystem::path ShaderDependencyManifest::GetPath(const std::filesystem::path& cookedShaderRoot)
{
	return cookedShaderRoot / "ShaderDependencies.sdep";
}

ShaderDependencyManifest ShaderDependencyManifest::Read(const std::filesystem::path& path)
{
	std::vector<std::uint8_t> bytes;
	std::string fileError;
	if (!Files::TryReadAllBytes(path, bytes, fileError))
	{
		throw Diagnostics::Error(
		    "Changed shader cooking requires valid dependency metadata. Run RecompileShaders Global to rebuild all shaders. " + fileError);
	}

	std::istringstream input{std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size())};
	std::string header;
	std::getline(input, header);
	if (header != kShaderDependencyManifestHeader)
	{
		throw Diagnostics::Error("Shader dependency metadata is invalid. Run RecompileShaders Global to rebuild all shaders.");
	}

	ShaderDependencyManifest manifest;
	std::unordered_map<ShaderTypeId, std::size_t> recordIndices;
	std::vector<std::pair<std::string, ShaderTypeId>> persistedReverseDependencies;
	std::string line;
	while (std::getline(input, line))
	{
		if (line.empty())
		{
			continue;
		}

		std::istringstream lineInput(line);
		std::string kind;
		lineInput >> kind;
		if (kind == "shader")
		{
			ShaderDependencyRecord record;
			lineInput >> std::hex >> record.ShaderType >> std::quoted(record.ShaderTypeName) >> std::quoted(record.VirtualSourcePath);
			lineInput >> std::ws;
			if (!lineInput || !lineInput.eof() || record.ShaderType == 0
			    || !recordIndices.emplace(record.ShaderType, manifest.m_records.size()).second)
			{
				throw Diagnostics::Error(
				    "Shader dependency metadata contains an invalid or duplicate shader record. Run RecompileShaders Global.");
			}
			manifest.m_records.push_back(std::move(record));
			continue;
		}

		if (kind == "dependency")
		{
			ShaderTypeId shaderType = 0;
			std::string virtualPath;
			lineInput >> std::hex >> shaderType >> std::quoted(virtualPath);
			lineInput >> std::ws;
			const auto record = recordIndices.find(shaderType);
			if (!lineInput || !lineInput.eof() || record == recordIndices.end())
			{
				throw Diagnostics::Error("Shader dependency metadata contains an orphan dependency. Run RecompileShaders Global.");
			}
			manifest.m_records[record->second].VirtualDependencies.push_back(std::move(virtualPath));
			continue;
		}

		if (kind == "reverse")
		{
			std::string virtualPath;
			ShaderTypeId shaderType = 0;
			lineInput >> std::quoted(virtualPath) >> std::hex >> shaderType;
			lineInput >> std::ws;
			if (!lineInput || !lineInput.eof())
			{
				throw Diagnostics::Error("Shader dependency metadata contains an invalid reverse dependency. Run RecompileShaders Global.");
			}
			persistedReverseDependencies.emplace_back(std::move(virtualPath), shaderType);
			continue;
		}

		throw Diagnostics::Error("Shader dependency metadata contains an unknown record. Run RecompileShaders Global.");
	}

	manifest.SortAndValidate();
	std::vector<std::pair<std::string, ShaderTypeId>> expectedReverseDependencies;
	for (const ShaderDependencyRecord& record : manifest.m_records)
	{
		for (const std::string& dependency : record.VirtualDependencies)
		{
			expectedReverseDependencies.emplace_back(dependency, record.ShaderType);
		}
	}
	std::ranges::sort(expectedReverseDependencies);
	std::ranges::sort(persistedReverseDependencies);
	if (persistedReverseDependencies != expectedReverseDependencies)
	{
		throw Diagnostics::Error("Shader dependency metadata forward and reverse records disagree. Run RecompileShaders Global.");
	}
	return manifest;
}

ShaderDependencyManifest ShaderDependencyManifest::ReadRequired(const std::filesystem::path& path)
{
	return Read(path);
}

std::optional<ShaderDependencyManifest> ShaderDependencyManifest::ReadForUpdate(const std::filesystem::path& path)
{
	std::error_code errorCode;
	const bool exists = std::filesystem::exists(path, errorCode);
	if (errorCode)
	{
		throw Diagnostics::Error(std::format("Failed to inspect shader dependency metadata '{}': {}", path.string(), errorCode.message()));
	}
	if (!exists)
	{
		return std::nullopt;
	}
	return Read(path);
}

void ShaderDependencyManifest::Write(const ShaderDependencyManifest& manifest, const std::filesystem::path& path)
{
	const std::filesystem::path temporaryPath = Files::BuildTemporaryPath(path);
	std::ofstream output;
	std::string fileError;
	if (!Files::TryOpenTextOutput(temporaryPath, output, fileError))
	{
		throw Diagnostics::Error(std::move(fileError));
	}

	output << kShaderDependencyManifestHeader << '\n';
	for (const ShaderDependencyRecord& record : manifest.m_records)
	{
		output << "shader " << std::hex << record.ShaderType << ' ' << std::quoted(record.ShaderTypeName) << ' '
		       << std::quoted(record.VirtualSourcePath) << '\n';
		for (const std::string& dependency : record.VirtualDependencies)
		{
			output << "dependency " << std::hex << record.ShaderType << ' ' << std::quoted(dependency) << '\n';
		}
	}
	for (const ShaderDependencyRecord& record : manifest.m_records)
	{
		for (const std::string& dependency : record.VirtualDependencies)
		{
			output << "reverse " << std::quoted(dependency) << ' ' << std::hex << record.ShaderType << '\n';
		}
	}

	if (!output.good())
	{
		output.close();
		Files::CleanupTemporaryFile(temporaryPath);
		throw Diagnostics::Error("Failed to write shader dependency metadata '" + path.string() + "'.");
	}
	if (!Files::TryCloseOutput(output, temporaryPath, fileError))
	{
		Files::CleanupTemporaryFile(temporaryPath);
		throw Diagnostics::Error(std::move(fileError));
	}
	if (!Files::TryFinalizeTemporaryFile(temporaryPath, path, fileError))
	{
		Files::CleanupTemporaryFile(temporaryPath);
		throw Diagnostics::Error(std::move(fileError));
	}
}

std::unordered_set<ShaderTypeId> ShaderDependencyManifest::SelectAffectedShaderTypes(std::span<const std::string> changedVirtualPaths) const
{
	std::unordered_set<std::string> changed(changedVirtualPaths.begin(), changedVirtualPaths.end());
	std::unordered_set<ShaderTypeId> affected;
	for (const ShaderDependencyRecord& record : m_records)
	{
		if (std::ranges::any_of(record.VirtualDependencies, [&changed](const std::string& path) { return changed.contains(path); }))
		{
			affected.insert(record.ShaderType);
		}
	}
	return affected;
}

void ShaderDependencyManifest::Replace(ShaderDependencyRecord record)
{
	const auto existing = std::ranges::find_if(
	    m_records,
	    [shaderType = record.ShaderType](const ShaderDependencyRecord& candidate) { return candidate.ShaderType == shaderType; });
	if (existing == m_records.end())
	{
		m_records.push_back(std::move(record));
	}
	else
	{
		*existing = std::move(record);
	}
}

std::size_t ShaderDependencyManifest::RemoveUnregisteredShaderTypes(std::span<const ShaderTypeId> registeredShaderTypes)
{
	if (!std::ranges::is_sorted(registeredShaderTypes)
	    || std::adjacent_find(registeredShaderTypes.begin(), registeredShaderTypes.end()) != registeredShaderTypes.end())
	{
		throw Diagnostics::Error("Current shader catalog identities must be sorted and unique before dependency reconciliation.");
	}
	const std::size_t previousCount = m_records.size();
	std::erase_if(
	    m_records,
	    [registeredShaderTypes](const ShaderDependencyRecord& record)
	    { return !std::ranges::binary_search(registeredShaderTypes, record.ShaderType); });
	return previousCount - m_records.size();
}

bool ShaderDependencyManifest::MatchesShaderTypes(std::span<const ShaderTypeId> shaderTypes) const noexcept
{
	return shaderTypes.size() == m_records.size()
	    && std::equal(
	        shaderTypes.begin(),
	        shaderTypes.end(),
	        m_records.begin(),
	        [](ShaderTypeId shaderType, const ShaderDependencyRecord& record) { return shaderType == record.ShaderType; });
}

void ShaderDependencyManifest::SortAndValidate()
{
	for (ShaderDependencyRecord& record : m_records)
	{
		if (record.ShaderType == 0 || record.ShaderTypeName.empty() || record.VirtualSourcePath.empty())
		{
			throw Diagnostics::Error("Shader dependency metadata contains an incomplete shader record.");
		}
		std::ranges::sort(record.VirtualDependencies);
		record.VirtualDependencies.erase(
		    std::unique(record.VirtualDependencies.begin(), record.VirtualDependencies.end()),
		    record.VirtualDependencies.end());
		if (record.VirtualDependencies.empty() || !std::ranges::binary_search(record.VirtualDependencies, record.VirtualSourcePath))
		{
			throw Diagnostics::Error(
			    std::format(
			        "Shader dependency metadata for '{}' omits its root virtual source '{}'.",
			        record.ShaderTypeName,
			        record.VirtualSourcePath));
		}
	}
	std::ranges::sort(
	    m_records,
	    [](const ShaderDependencyRecord& lhs, const ShaderDependencyRecord& rhs) { return lhs.ShaderType < rhs.ShaderType; });
	const auto duplicate = std::adjacent_find(
	    m_records.begin(),
	    m_records.end(),
	    [](const ShaderDependencyRecord& lhs, const ShaderDependencyRecord& rhs) { return lhs.ShaderType == rhs.ShaderType; });
	if (duplicate != m_records.end())
	{
		throw Diagnostics::Error("Shader dependency metadata contains duplicate shader type ids.");
	}
}
