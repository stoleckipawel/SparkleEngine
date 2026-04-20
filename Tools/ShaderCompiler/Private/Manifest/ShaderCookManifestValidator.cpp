#include "PCH.h"

#include "Manifest/ShaderCookManifestValidator.h"

#include "Core/Public/Assets/AssetTypes.h"
#include "Core/Public/FileSystemUtils.h"

#include <set>

bool ShaderCookManifestValidator::Validate(
	const ShaderCookPackageDesc& package,
	const std::filesystem::path& manifestPath,
	std::string& outErrorMessage)
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
