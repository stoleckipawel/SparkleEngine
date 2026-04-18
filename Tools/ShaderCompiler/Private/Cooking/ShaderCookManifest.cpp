#include "PCH.h"

#include "Cooking/ShaderCookManifest.h"

#include "Core/Public/Assets/AssetTypes.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Strings/StringUtils.h"

#include <algorithm>
#include <format>
#include <fstream>
#include <map>
#include <optional>
#include <set>
#include <system_error>

namespace
{
	using Engine::AssetAuthoring::ShaderCookPackageDesc;
	using Engine::AssetAuthoring::ShaderCookStageDesc;

	constexpr std::string_view kManifestHeader = "ShaderCookManifest";

	std::optional<ShaderStage> TryParseShaderStage(std::string_view value) noexcept
	{
		if (Engine::Strings::EqualsIgnoreCase(value, "Vertex"))
		{
			return ShaderStage::Vertex;
		}

		if (Engine::Strings::EqualsIgnoreCase(value, "Pixel"))
		{
			return ShaderStage::Pixel;
		}

		if (Engine::Strings::EqualsIgnoreCase(value, "Geometry"))
		{
			return ShaderStage::Geometry;
		}

		if (Engine::Strings::EqualsIgnoreCase(value, "Hull"))
		{
			return ShaderStage::Hull;
		}

		if (Engine::Strings::EqualsIgnoreCase(value, "Domain"))
		{
			return ShaderStage::Domain;
		}

		if (Engine::Strings::EqualsIgnoreCase(value, "Compute"))
		{
			return ShaderStage::Compute;
		}

		return std::nullopt;
	}

	bool ParseStageValue(std::string_view value, ShaderCookStageDesc& outStage, std::string& outErrorMessage)
	{
		const std::size_t separatorIndex = value.find('|');
		if (separatorIndex == std::string_view::npos)
		{
			outStage.sourcePath = Engine::Strings::UnquoteCopy(value);
			outStage.entryPoint = "main";
		}
		else
		{
			outStage.sourcePath = Engine::Strings::UnquoteCopy(value.substr(0, separatorIndex));
			outStage.entryPoint = Engine::Strings::UnquoteCopy(value.substr(separatorIndex + 1));
		}

		if (outStage.sourcePath.empty())
		{
			outErrorMessage = "Shader stage entry is missing a source path.";
			return false;
		}

		if (outStage.entryPoint.empty())
		{
			outErrorMessage = "Shader stage entry is missing an entry point.";
			return false;
		}

		outErrorMessage.clear();
		return true;
	}

	std::string MakePackageLookupKey(std::string_view packageId)
	{
		return Engine::Strings::ToLowerCopy(Engine::Strings::TrimAsciiWhitespace(packageId));
	}

	bool ValidatePackage(const ShaderCookPackageDesc& package, const std::filesystem::path& manifestPath, std::string& outErrorMessage)
	{
		if (package.packageId.empty())
		{
			outErrorMessage = "Shader package manifest contains an entry with no package id: '" + manifestPath.string() + "'";
			return false;
		}

		if (package.bindingLayoutId.empty())
		{
			outErrorMessage = "Shader package '" + package.packageId + "' is missing BindingLayout in '" + manifestPath.string() + "'";
			return false;
		}

		if (package.stages.empty())
		{
			outErrorMessage = "Shader package '" + package.packageId + "' declares no stages in '" + manifestPath.string() + "'";
			return false;
		}

		std::set<ShaderStage> declaredStages;
		for (const ShaderCookStageDesc& stage : package.stages)
		{
			if (stage.stage == ShaderStage::Count)
			{
				outErrorMessage = "Shader package '" + package.packageId + "' contains an invalid shader stage in '" + manifestPath.string() + "'";
				return false;
			}

			if (!declaredStages.insert(stage.stage).second)
			{
				outErrorMessage = "Shader package '" + package.packageId + "' declares the same shader stage more than once in '" + manifestPath.string() + "'";
				return false;
			}

			if (!Filesystem::ResolveAssetPathNormalized(stage.sourcePath, AssetType::Shader))
			{
				outErrorMessage = "Shader package '" + package.packageId + "' references a missing shader asset '" + stage.sourcePath.string() + "'";
				return false;
			}
		}

		outErrorMessage.clear();
		return true;
	}

	bool ParseManifestFile(
	    const std::filesystem::path& manifestPath,
	    std::map<std::string, ShaderCookPackageDesc, std::less<>>& inOutPackages,
	    std::string& outErrorMessage)
	{
		std::ifstream input(manifestPath);
		if (!input.is_open())
		{
			outErrorMessage = "Failed to open shader cook manifest '" + manifestPath.string() + "'";
			return false;
		}

		bool inManifestSection = false;
		std::optional<ShaderCookPackageDesc> currentPackage;
		std::size_t lineNumber = 0;

		auto flushCurrentPackage = [&]() -> bool
		{
			if (!currentPackage.has_value())
			{
				return true;
			}

			if (!ValidatePackage(*currentPackage, manifestPath, outErrorMessage))
			{
				return false;
			}

			inOutPackages[MakePackageLookupKey(currentPackage->packageId)] = *currentPackage;
			currentPackage.reset();
			return true;
		};

		for (std::string line; std::getline(input, line);)
		{
			++lineNumber;
			const std::string trimmedLine = Engine::Strings::TrimCopy(line);
			if (trimmedLine.empty() || trimmedLine[0] == '#' || trimmedLine[0] == ';')
			{
				continue;
			}

			if (trimmedLine.front() == '[' && trimmedLine.back() == ']')
			{
				if (!flushCurrentPackage())
				{
					return false;
				}

				const std::string_view sectionName = Engine::Strings::TrimAsciiWhitespace(
				    std::string_view(trimmedLine).substr(1, trimmedLine.size() - 2));

				if (Engine::Strings::EqualsIgnoreCase(sectionName, kManifestHeader))
				{
					inManifestSection = true;
					continue;
				}

				constexpr std::string_view kPackagePrefix = "Package ";
				if (sectionName.size() > kPackagePrefix.size() &&
				    Engine::Strings::EqualsIgnoreCase(sectionName.substr(0, kPackagePrefix.size()), kPackagePrefix))
				{
					inManifestSection = false;
					currentPackage.emplace();
					currentPackage->packageId = Engine::Strings::TrimCopy(sectionName.substr(kPackagePrefix.size()));
					continue;
				}

				outErrorMessage = "Unknown section '" + std::string(sectionName) + "' in shader cook manifest '" + manifestPath.string() + "'";
				return false;
			}

			std::string_view key;
			std::string_view value;
			if (!Engine::Strings::TrySplitKeyValue(trimmedLine, '=', key, value))
			{
				outErrorMessage = "Malformed line " + std::to_string(lineNumber) + " in shader cook manifest '" + manifestPath.string() + "'";
				return false;
			}

			key = Engine::Strings::TrimAsciiWhitespace(key);
			value = Engine::Strings::TrimAsciiWhitespace(value);

			if (inManifestSection)
			{
				if (!Engine::Strings::EqualsIgnoreCase(key, "Version"))
				{
					continue;
				}

				continue;
			}

			if (!currentPackage.has_value())
			{
				outErrorMessage = "Key-value data must appear inside a package section in shader cook manifest '" + manifestPath.string() + "'";
				return false;
			}

			if (Engine::Strings::EqualsIgnoreCase(key, "BindingLayout"))
			{
				currentPackage->bindingLayoutId = Engine::Strings::UnquoteCopy(value);
				continue;
			}

			if (Engine::Strings::EqualsIgnoreCase(key, "Variant"))
			{
				currentPackage->variantId = Engine::Strings::UnquoteCopy(value);
				continue;
			}

			constexpr std::string_view kStagePrefix = "Stage.";
			if (key.size() > kStagePrefix.size() &&
			    Engine::Strings::EqualsIgnoreCase(key.substr(0, kStagePrefix.size()), kStagePrefix))
			{
				const std::optional<ShaderStage> stage = TryParseShaderStage(key.substr(kStagePrefix.size()));
				if (!stage.has_value())
				{
					outErrorMessage = "Unknown shader stage key '" + std::string(key) + "' in shader cook manifest '" + manifestPath.string() + "'";
					return false;
				}

				ShaderCookStageDesc stageDesc;
				stageDesc.stage = *stage;
				if (!ParseStageValue(value, stageDesc, outErrorMessage))
				{
					outErrorMessage += " Manifest: '" + manifestPath.string() + "'";
					return false;
				}

				currentPackage->stages.push_back(std::move(stageDesc));
				continue;
			}

			outErrorMessage = "Unknown key '" + std::string(key) + "' in shader cook manifest '" + manifestPath.string() + "'";
			return false;
		}

		return flushCurrentPackage();
	}
}  // namespace

namespace Engine::AssetAuthoring
{
	bool ShaderCookManifest::LoadMerged(std::string& outErrorMessage)
	{
		m_packages.clear();

		std::map<std::string, ShaderCookPackageDesc, std::less<>> mergedPackages;
		bool loadedAnyManifest = false;

		const std::filesystem::path engineManifestPath = GetEngineManifestPath();
		const std::filesystem::path projectManifestPath = GetProjectManifestPath();

		std::error_code errorCode;
		if (!engineManifestPath.empty() && std::filesystem::exists(engineManifestPath, errorCode))
		{
			if (!ParseManifestFile(engineManifestPath, mergedPackages, outErrorMessage))
			{
				return false;
			}

			loadedAnyManifest = true;
		}

		errorCode.clear();
		if (!projectManifestPath.empty() && std::filesystem::exists(projectManifestPath, errorCode))
		{
			if (!ParseManifestFile(projectManifestPath, mergedPackages, outErrorMessage))
			{
				return false;
			}

			loadedAnyManifest = true;
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

	std::filesystem::path ShaderCookManifest::GetCookedShaderRoot()
	{
		return GetCookedShaderRootPath();
	}

	std::filesystem::path ShaderCookManifest::GetCookedShaderPackageRoot()
	{
		return GetCookedShaderPackageRootPath();
	}

	std::filesystem::path ShaderCookManifest::GetCookedShaderRegistryPath()
	{
		return ::GetCookedShaderRegistryPath();
	}

	std::uint64_t ShaderCookManifest::BuildShaderPackageKey(std::string_view packageId, std::string_view variantId)
	{
		return ::BuildShaderPackageKey(packageId, variantId);
	}

	std::filesystem::path ShaderCookManifest::BuildCookedShaderPackagePath(std::uint64_t packageKey)
	{
		return ::BuildCookedShaderPackagePath(packageKey);
	}
}  // namespace Engine::AssetAuthoring