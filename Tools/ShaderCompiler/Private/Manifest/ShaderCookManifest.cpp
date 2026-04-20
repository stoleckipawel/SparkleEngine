#include "PCH.h"

#include "Manifest/ShaderCookManifest.h"

#include "Core/Public/Assets/AssetTypes.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Strings/StringUtils.h"
#include "Manifest/ShaderCookManifestParser.h"

#include <algorithm>
#include <system_error>

std::filesystem::path ShaderCookManifest::GetEngineManifestPath()
{
	const std::filesystem::path& shaderRoot = Filesystem::GetTypedPath(AssetType::Shader, PathRoot::Engine);
	return shaderRoot.empty() ? std::filesystem::path{} : shaderRoot / kShaderCookManifestFileName;
}

std::filesystem::path ShaderCookManifest::GetProjectManifestPath()
{
	const std::filesystem::path& shaderRoot = Filesystem::GetTypedPath(AssetType::Shader, PathRoot::Project);
	return shaderRoot.empty() ? std::filesystem::path{} : shaderRoot / kShaderCookManifestFileName;
}

bool ShaderCookManifest::LoadMerged(std::string& outErrorMessage)
	{
		m_packages.clear();

		ShaderCookPackageMap mergedPackages;
		bool loadedAnyManifest = false;

		auto loadIfExists = [&](const std::filesystem::path& path) -> bool
		{
			if (path.empty())
			{
				return true;
			}
			std::error_code errorCode;
			if (!std::filesystem::exists(path, errorCode))
			{
				return true;
			}
			if (!ShaderCookManifestParser::ParseInto(path, mergedPackages, outErrorMessage))
			{
				return false;
			}
			loadedAnyManifest = true;
			return true;
		};

		if (!loadIfExists(GetEngineManifestPath()))
		{
			return false;
		}
		if (!loadIfExists(GetProjectManifestPath()))
		{
			return false;
		}

		if (!loadedAnyManifest)
		{
			outErrorMessage = "No shader cook manifest was found under the engine or project shader roots.";
			return false;
		}

		m_packages.reserve(mergedPackages.size());
		for (auto& [_, package] : mergedPackages)
		{
			m_packages.push_back(std::move(package));
		}

		std::sort(
		    m_packages.begin(),
		    m_packages.end(),
		    [](const ShaderCookPackageDesc& lhs, const ShaderCookPackageDesc& rhs)
		    {
			    return Engine::Strings::ToLowerCopy(lhs.packageId) < Engine::Strings::ToLowerCopy(rhs.packageId);
		    });

		outErrorMessage.clear();
		return true;
	}
