#pragma once

#include "ShaderContractCatalog.h"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <unordered_set>
#include <vector>

struct ShaderDependencyRecord final
{
	ShaderTypeId ShaderType = 0;
	std::string ShaderTypeName;
	std::string VirtualSourcePath;
	std::vector<std::string> VirtualDependencies;
};

class ShaderDependencyManifest final
{
public:
	static std::filesystem::path GetPath(const std::filesystem::path& cookedShaderRoot);
	static ShaderDependencyManifest ReadRequired(const std::filesystem::path& path);
	static std::optional<ShaderDependencyManifest> ReadForUpdate(const std::filesystem::path& path);
	static void Write(const ShaderDependencyManifest& manifest, const std::filesystem::path& path);

	std::unordered_set<ShaderTypeId> SelectAffectedShaderTypes(std::span<const std::string> changedVirtualPaths) const;
	void Replace(ShaderDependencyRecord record);
	std::size_t RemoveUnregisteredShaderTypes(std::span<const ShaderTypeId> registeredShaderTypes);
	void SortAndValidate();
	bool MatchesShaderTypes(std::span<const ShaderTypeId> shaderTypes) const noexcept;

private:
	static ShaderDependencyManifest Read(const std::filesystem::path& path);

	std::vector<ShaderDependencyRecord> m_records;
};
