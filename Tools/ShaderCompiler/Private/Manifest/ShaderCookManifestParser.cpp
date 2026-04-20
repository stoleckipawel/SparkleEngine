#include "PCH.h"

#include "Manifest/ShaderCookManifestParser.h"

#include "Constants/ShaderCompilerConstants.h"
#include "Core/Public/Strings/StringUtils.h"
#include "Manifest/ShaderCookManifestValidator.h"
#include "Manifest/ShaderStageNames.h"

#include <fstream>
#include <optional>
#include <string>

std::string ShaderCookManifestParser::MakePackageLookupKey(std::string_view packageId)
{
	return Engine::Strings::ToLowerCopy(Engine::Strings::TrimAsciiWhitespace(packageId));
}

bool ShaderCookManifestParser::ParseStageValue(
    std::string_view value,
    ShaderCookStageDesc& outStage,
    std::string& outErrorMessage)
	{
		const std::size_t separatorIndex = value.find(kManifestStageValueSeparator);
		if (separatorIndex == std::string_view::npos)
		{
			outStage.sourcePath = Engine::Strings::UnquoteCopy(value);
			outStage.entryPoint = std::string{kManifestDefaultEntryPoint};
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

bool ShaderCookManifestParser::ParseInto(
    const std::filesystem::path& manifestPath,
    ShaderCookPackageMap& inOutPackages,
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
			if (!ShaderCookManifestValidator::Validate(*currentPackage, manifestPath, outErrorMessage))
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

				if (Engine::Strings::EqualsIgnoreCase(sectionName, kManifestHeaderSection))
				{
					inManifestSection = true;
					continue;
				}

				if (sectionName.size() > kManifestPackageSectionPrefix.size() &&
				    Engine::Strings::EqualsIgnoreCase(
				        sectionName.substr(0, kManifestPackageSectionPrefix.size()),
				        kManifestPackageSectionPrefix))
				{
					inManifestSection = false;
					currentPackage.emplace();
					currentPackage->packageId = Engine::Strings::TrimCopy(
					    sectionName.substr(kManifestPackageSectionPrefix.size()));
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
				// Header section currently only carries an optional Version key; ignore others silently
				// to allow forward-compatible additions without breaking older tools.
				continue;
			}

			if (!currentPackage.has_value())
			{
				outErrorMessage = "Key-value data must appear inside a package section in shader cook manifest '" + manifestPath.string() + "'";
				return false;
			}

			if (Engine::Strings::EqualsIgnoreCase(key, kManifestKeyBindingLayout))
			{
				currentPackage->bindingLayoutId = Engine::Strings::UnquoteCopy(value);
				continue;
			}

			if (Engine::Strings::EqualsIgnoreCase(key, kManifestKeyVariant))
			{
				currentPackage->variantId = Engine::Strings::UnquoteCopy(value);
				continue;
			}

			if (key.size() > kManifestStageKeyPrefix.size() &&
			    Engine::Strings::EqualsIgnoreCase(
			        key.substr(0, kManifestStageKeyPrefix.size()),
			        kManifestStageKeyPrefix))
			{
				const std::optional<ShaderStage> stage =
				    ShaderStageNames::TryParse(key.substr(kManifestStageKeyPrefix.size()));
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
