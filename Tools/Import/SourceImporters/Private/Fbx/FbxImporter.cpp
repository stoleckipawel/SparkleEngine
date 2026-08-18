#include "PCH.h"

#include "Fbx/FbxImporter.h"

#include "Fbx/FbxAnimationImporter.h"
#include "Fbx/FbxCameraImporter.h"
#include "Fbx/FbxEmbeddedTextureImporter.h"
#include "Fbx/FbxGeometryImporter.h"
#include "Fbx/FbxLightImporter.h"
#include "Fbx/FbxMaterialImporter.h"
#include "Fbx/FbxSceneReader.h"
#include "Core/Public/Diagnostics/Error.h"

#include <assimp/Importer.hpp>

#include <format>
#include <vector>

std::string_view FbxImporter::GetImporterId() const noexcept
{
	return "FbxImporter";
}

bool FbxImporter::SupportsExtension(std::wstring_view extension) const noexcept
{
	return extension == L".fbx";
}

SourceImportOutput FbxImporter::Import(const std::filesystem::path& filePath) const
{
	SourceImportOutput output;
	output.provenance.sourcePath = filePath;
	output.provenance.importerId = std::string(GetImporterId());

	Assimp::Importer importer;
	const aiScene& scene = FbxSceneReader::LoadScene(filePath, importer);
	const float sourceMetersPerUnit = FbxSceneReader::GetMetersPerSourceUnit(importer);
	output.provenance.sourceMetersPerUnit = sourceMetersPerUnit;
	output.scene.materials.reserve(scene.mNumMaterials);
	const std::size_t importedMeshInstanceCount = FbxGeometryImporter::CountImportedMeshInstances(*scene.mRootNode);
	output.ReserveMeshPrimitives(scene.mNumMeshes);
	output.ReserveMeshInstances(importedMeshInstanceCount);

	const std::vector<std::filesystem::path> embeddedTexturePaths = FbxEmbeddedTextureImporter::ExtractTextures(scene);
	FbxMaterialImporter::ImportMaterials(scene, filePath.parent_path(), embeddedTexturePaths, output);

	FbxGeometryImporter::ImportGeometry(scene, output);
	FbxCameraImporter::ImportCameras(scene, sourceMetersPerUnit, output);
	FbxLightImporter::ImportLights(scene, sourceMetersPerUnit, output);
	FbxAnimationImporter::ImportAnimations(scene, output);

	if (output.scene.meshPrimitives.empty() != output.scene.meshInstances.empty()
	    || (output.scene.meshPrimitives.empty() && output.scene.cameras.empty() && output.scene.lights.empty()
	        && output.scene.animations.empty()))
	{
		throw Diagnostics::Error("FBX import produced incomplete mesh content or no supported scene content.");
	}

	return output;
}
