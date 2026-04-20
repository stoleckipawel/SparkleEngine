#pragma once

#include "Manifest/ShaderCookManifestTypes.h"

#include <filesystem>
#include <string>
#include <vector>

class ShaderCookManifest final
{
  public:
	bool LoadMerged(std::string& outErrorMessage);

	const std::vector<ShaderCookPackageDesc>& GetPackages() const noexcept { return m_packages; }

	static std::filesystem::path GetEngineManifestPath();
	static std::filesystem::path GetProjectManifestPath();

  private:
	std::vector<ShaderCookPackageDesc> m_packages;
};
