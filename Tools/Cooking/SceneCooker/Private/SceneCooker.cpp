#include "PCH.h"

#include "SceneCooker.h"

#include "Features/Cameras/CookedSceneCameraBuilder.h"
#include "Core/Public/Assets/AssetTypes.h"
#include "Core/Public/Files/BinaryStreamWriter.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Assets/SceneAssetRegistry.h"

#include <fstream>
#include <limits>
#include <optional>

bool SceneCooker::ResolveSceneIdentity(
    const std::filesystem::path& sourceScenePath,
	CookedSceneIdentity& outIdentity,
    std::string& outErrorMessage)
{
	std::filesystem::path resolvedSourceScenePath;
	if (!ResolveSourceScenePath(sourceScenePath, resolvedSourceScenePath, outErrorMessage))
	{
		return false;
	}

	if (!BuildSceneAssetId(resolvedSourceScenePath, outIdentity.assetId, outErrorMessage))
	{
		return false;
	}

	outIdentity.manifestPath = Paths::CookedSceneManifest(outIdentity.assetId);
	outErrorMessage.clear();
	return true;
}

bool SceneCooker::BuildManifest(
    const SourceImportResult& importResult,
    CookedSceneBuild& outBuild,
    std::string& outErrorMessage)
{
	outBuild.manifest.instances.clear();
	outBuild.manifest.instances.reserve(importResult.scene.meshInstances.size());
	outBuild.manifest.instanceGroups.clear();
	outBuild.manifest.instanceGroups.reserve(importResult.scene.meshInstanceGroups.size());

	for (std::size_t instanceIndex = 0; instanceIndex < importResult.scene.meshInstances.size(); ++instanceIndex)
	{
		const ImportedMeshInstance& importedInstance = importResult.scene.meshInstances[instanceIndex];
		if (!importedInstance.HasPrimitiveBinding() || importedInstance.primitiveIndex >= outBuild.manifest.meshAssetReferences.size())
		{
			outErrorMessage = "Imported mesh instance references a primitive index outside the cooked mesh asset set";
			return false;
		}

		std::uint32_t materialAssetIndex = Assets::kInvalidCookedMaterialAssetIndex;
		if (importedInstance.HasMaterialBinding())
		{
			materialAssetIndex = importedInstance.materialIndex;
			if (materialAssetIndex >= outBuild.outputs.materialAssets.size())
			{
				outErrorMessage = "Imported mesh instance references a material index outside the imported material set";
				return false;
			}
		}

		std::uint32_t groupIndex = Assets::kInvalidCookedSceneInstanceGroupIndex;
		if (importedInstance.groupIndex != kInvalidImportedMeshInstanceGroupIndex)
		{
			if (importedInstance.groupIndex >= importResult.scene.meshInstanceGroups.size())
			{
				outErrorMessage = "Imported mesh instance references an instance group outside the imported group set";
				return false;
			}

			groupIndex = importedInstance.groupIndex;
		}

		outBuild.manifest.instances.push_back(
		    Assets::CookedSceneInstanceRecord{
		        .meshAssetIndex = importedInstance.primitiveIndex,
		        .materialAssetIndex = materialAssetIndex,
		        .groupIndex = groupIndex,
		        .worldTransform = importedInstance.worldTransform});
	}

	for (std::size_t groupIndex = 0; groupIndex < importResult.scene.meshInstanceGroups.size(); ++groupIndex)
	{
		const ImportedMeshInstanceGroup& importedGroup = importResult.scene.meshInstanceGroups[groupIndex];
		if (!importedGroup.HasPrimitiveBinding() || importedGroup.primitiveIndex >= outBuild.manifest.meshAssetReferences.size())
		{
			outErrorMessage = "Imported mesh instance group references a primitive index outside the cooked mesh asset set";
			return false;
		}

		std::uint32_t materialAssetIndex = Assets::kInvalidCookedMaterialAssetIndex;
		if (importedGroup.HasMaterialBinding())
		{
			materialAssetIndex = importedGroup.materialIndex;
			if (materialAssetIndex >= outBuild.outputs.materialAssets.size())
			{
				outErrorMessage = "Imported mesh instance group references a material index outside the imported material set";
				return false;
			}
		}

		if (!importedGroup.HasInstanceRange() || importedGroup.firstInstanceIndex >= outBuild.manifest.instances.size() ||
		    importedGroup.instanceCount > outBuild.manifest.instances.size() - importedGroup.firstInstanceIndex)
		{
			outErrorMessage = "Imported mesh instance group references an instance range outside the cooked instance set";
			return false;
		}

		if (groupIndex > (std::numeric_limits<std::uint32_t>::max)())
		{
			outErrorMessage = "Imported mesh instance group count exceeds the cooked scene manifest range";
			return false;
		}

		outBuild.manifest.instanceGroups.push_back(
		    Assets::CookedSceneInstanceGroupRecord{
		        .meshAssetIndex = importedGroup.primitiveIndex,
		        .materialAssetIndex = materialAssetIndex,
		        .firstInstance = importedGroup.firstInstanceIndex,
		        .instanceCount = importedGroup.instanceCount,
		        .groupKind = ToCookedInstanceGroupKind(importedGroup.groupKind),
		        .flags = importedGroup.flags});
	}

	CookedSceneCameraBuilder::BuildCameras(importResult, outBuild);

	outBuild.manifest.header.meshAssetReferenceCount = static_cast<std::uint32_t>(outBuild.manifest.meshAssetReferences.size());
	outBuild.manifest.header.materialAssetReferenceCount = static_cast<std::uint32_t>(outBuild.manifest.materialAssetReferences.size());
	outBuild.manifest.header.instanceCount = static_cast<std::uint32_t>(outBuild.manifest.instances.size());
	outBuild.manifest.header.instanceGroupCount = static_cast<std::uint32_t>(outBuild.manifest.instanceGroups.size());
	outBuild.manifest.header.cameraCount = static_cast<std::uint32_t>(outBuild.manifest.cameras.size());
	outErrorMessage.clear();
	return true;
}

bool SceneCooker::WriteSceneManifestAndRegistry(const CookedSceneBuild& build, std::string& outErrorMessage)
{
	std::ofstream manifestOutput;
	if (!Files::TryOpenBinaryOutput(build.identity.manifestPath, manifestOutput, outErrorMessage))
	{
		return false;
	}

	if (!Files::BinaryStreamWriter::WriteValue(manifestOutput, build.manifest.header, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.meshAssetReferences, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.materialAssetReferences, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.instances, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.instanceGroups, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.cameras, outErrorMessage))
	{
		return false;
	}

	if (!Files::TryCloseOutput(manifestOutput, build.identity.manifestPath, outErrorMessage))
	{
		return false;
	}

	if (!UpdateSceneAssetRegistry(build, outErrorMessage))
	{
		return false;
	}

	outErrorMessage.clear();
	return true;
}

bool SceneCooker::ResolveSourceScenePath(
    const std::filesystem::path& sourceScenePath,
    std::filesystem::path& outResolvedPath,
    std::string& outErrorMessage)
{
	if (const auto resolvedPath = Filesystem::ResolveAssetPathNormalized(sourceScenePath, AssetType::Mesh))
	{
		outResolvedPath = *resolvedPath;
		outErrorMessage.clear();
		return true;
	}

	outErrorMessage = "Unable to resolve source scene path '" + sourceScenePath.string() + "'";
	return false;
}

bool SceneCooker::BuildSceneAssetId(
    const std::filesystem::path& resolvedSourceScenePath,
    std::string& outSceneAssetId,
    std::string& outErrorMessage)
{
	const std::filesystem::path projectMeshRoot = Paths::TypedAssetRoot(AssetType::Mesh, PathRoot::Project);
	const std::filesystem::path engineMeshRoot = Paths::TypedAssetRoot(AssetType::Mesh, PathRoot::Engine);

	std::optional<std::filesystem::path> relativePath = Paths::TryMakeRelativeUnderRoot(resolvedSourceScenePath, projectMeshRoot);
	if (!relativePath)
	{
		relativePath = Paths::TryMakeRelativeUnderRoot(resolvedSourceScenePath, engineMeshRoot);
	}

	if (!relativePath)
	{
		outErrorMessage = "Source scene path must be under a Sparkle mesh asset root to derive a stable scene asset id: '" +
		                  resolvedSourceScenePath.string() + "'";
		return false;
	}

	outSceneAssetId = relativePath->generic_string();
	std::filesystem::path sceneAssetPath(outSceneAssetId);
	sceneAssetPath.replace_extension();
	outSceneAssetId = sceneAssetPath.generic_string();
	outErrorMessage.clear();
	return true;
}

bool SceneCooker::UpdateSceneAssetRegistry(const CookedSceneBuild& build, std::string& outErrorMessage)
{
	const std::filesystem::path manifestRoot = Paths::CookedSceneManifestRoot();
	const std::optional<std::filesystem::path> manifestRelativePath =
	    Paths::TryMakeRelativeUnderRoot(build.identity.manifestPath, manifestRoot);
	if (!manifestRelativePath)
	{
		outErrorMessage = "Failed to derive a relative cooked scene manifest path for scene asset id '" + build.identity.assetId + "'";
		return false;
	}

	Assets::SceneAssetRegistry sceneAssetRegistry;
	if (!sceneAssetRegistry.Load(outErrorMessage))
	{
		return false;
	}

	sceneAssetRegistry.Upsert(build.identity.assetId, *manifestRelativePath);
	if (!sceneAssetRegistry.Save(outErrorMessage))
	{
		return false;
	}

	outErrorMessage.clear();
	return true;
}

Assets::CookedSceneInstanceGroupKind SceneCooker::ToCookedInstanceGroupKind(ImportedMeshInstanceGroupKind groupKind) noexcept
{
	switch (groupKind)
	{
		case ImportedMeshInstanceGroupKind::SharedMeshReference:
			return Assets::CookedSceneInstanceGroupKind::SharedMeshReference;
		case ImportedMeshInstanceGroupKind::AuthoredInstanceGroup:
			return Assets::CookedSceneInstanceGroupKind::AuthoredInstanceGroup;
		case ImportedMeshInstanceGroupKind::None:
		default:
			return Assets::CookedSceneInstanceGroupKind::None;
	}
}
