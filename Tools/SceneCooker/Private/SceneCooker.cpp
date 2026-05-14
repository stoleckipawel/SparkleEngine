#include "PCH.h"

#include "SceneCooker.h"

#include "Core/Public/Assets/AssetTypes.h"
#include "Core/Public/Files/BinaryStreamWriter.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Assets/SceneAssetRegistry.h"

#include <fstream>
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
	outBuild.manifest.instances.reserve(importResult.scene.meshes.size());

	for (std::size_t meshIndex = 0; meshIndex < importResult.scene.meshes.size(); ++meshIndex)
	{
		const ImportedMesh& importedMesh = importResult.scene.meshes[meshIndex];

		std::uint32_t materialAssetIndex = Assets::kInvalidCookedMaterialAssetIndex;
		if (importedMesh.HasMaterialBinding())
		{
			materialAssetIndex = importedMesh.materialIndex;
			if (materialAssetIndex >= outBuild.outputs.materialAssets.size())
			{
				outErrorMessage = "Imported mesh instance references a material index outside the imported material set";
				return false;
			}
		}

		outBuild.manifest.instances.push_back(
		    Assets::CookedSceneInstanceRecord{
		        .meshAssetIndex = static_cast<std::uint32_t>(meshIndex),
		        .materialAssetIndex = materialAssetIndex,
		        .worldTransform = importedMesh.worldTransform});
	}

	outBuild.manifest.header.meshAssetReferenceCount = static_cast<std::uint32_t>(outBuild.manifest.meshAssetReferences.size());
	outBuild.manifest.header.materialAssetReferenceCount = static_cast<std::uint32_t>(outBuild.manifest.materialAssetReferences.size());
	outBuild.manifest.header.instanceCount = static_cast<std::uint32_t>(outBuild.manifest.instances.size());
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
	    !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.instances, outErrorMessage))
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
