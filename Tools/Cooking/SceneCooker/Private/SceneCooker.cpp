#include "PCH.h"

#include "SceneCooker.h"

#include "Features/Cameras/CookedSceneCameraBuilder.h"
#include "Features/Instances/CookedSceneInstanceBuilder.h"
#include "Features/Lights/CookedSceneLightBuilder.h"
#include "Features/Metadata/CookedSceneMetadataBuilder.h"
#include "Features/Skeletons/CookedSceneSkeletonBuilder.h"
#include "Core/Public/Assets/AssetTypes.h"
#include "Core/Public/Files/BinaryStreamWriter.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Assets/SceneAssetRegistry.h"

#include <fstream>
#include <optional>

namespace
{
	bool ValidateSceneMetadataCounts(
	    const SourceImportResult& importResult,
	    const CookedSceneBuild& build,
	    std::string& outErrorMessage)
	{
		if (build.manifest.cameras.size() != importResult.scene.cameras.size())
		{
			outErrorMessage = "Cooked scene camera metadata count does not match imported camera count";
			return false;
		}

		if (build.manifest.lights.size() != importResult.scene.lights.size())
		{
			outErrorMessage = "Cooked scene light metadata count does not match imported light count";
			return false;
		}

		if (build.manifest.skeletonRefs.size() != importResult.scene.skeletons.size())
		{
			outErrorMessage = "Cooked scene skeleton metadata count does not match imported skeleton count";
			return false;
		}

		if (!build.manifest.animationRefs.empty())
		{
			outErrorMessage = "Cooked scene animation refs are present before animation asset cooking is implemented";
			return false;
		}

		outErrorMessage.clear();
		return true;
	}
}

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
	outBuild.manifest.instanceGroups.clear();
	CookedSceneSkeletonBuilder::BuildSkeletons(importResult, outBuild.identity.assetId, outBuild);
	if (!CookedSceneInstanceBuilder::BuildInstances(importResult, outBuild, outErrorMessage))
	{
		return false;
	}

	CookedSceneCameraBuilder::BuildCameras(importResult, outBuild);
	CookedSceneLightBuilder::BuildLights(importResult, outBuild);
	CookedSceneMetadataBuilder::BuildMetadata(importResult, outBuild);
	if (!ValidateSceneMetadataCounts(importResult, outBuild, outErrorMessage))
	{
		return false;
	}

	outBuild.manifest.header.meshAssetReferenceCount = static_cast<std::uint32_t>(outBuild.manifest.meshAssetReferences.size());
	outBuild.manifest.header.materialAssetReferenceCount = static_cast<std::uint32_t>(outBuild.manifest.materialAssetReferences.size());
	outBuild.manifest.header.instanceCount = static_cast<std::uint32_t>(outBuild.manifest.instances.size());
	outBuild.manifest.header.instanceGroupCount = static_cast<std::uint32_t>(outBuild.manifest.instanceGroups.size());
	outBuild.manifest.header.cameraCount = static_cast<std::uint32_t>(outBuild.manifest.cameras.size());
	outBuild.manifest.header.lightCount = static_cast<std::uint32_t>(outBuild.manifest.lights.size());
	outBuild.manifest.header.skeletonRefCount = static_cast<std::uint32_t>(outBuild.manifest.skeletonRefs.size());
	outBuild.manifest.header.animationRefCount = static_cast<std::uint32_t>(outBuild.manifest.animationRefs.size());
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
	    !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.cameras, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.lights, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.skeletonRefs, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.animationRefs, outErrorMessage))
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
