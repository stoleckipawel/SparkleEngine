#pragma once

#include "ShaderContractCatalog.h"

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
	std::string PublicationGroup;
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
	void SortAndValidate();
	void SetCompleteCatalog(bool complete) noexcept { m_completeCatalog = complete; }
	bool IsCompleteCatalog() const noexcept { return m_completeCatalog; }

private:
	static ShaderDependencyManifest Read(const std::filesystem::path& path, bool requireComplete);

	std::vector<ShaderDependencyRecord> m_records;
	bool m_completeCatalog = false;
};
