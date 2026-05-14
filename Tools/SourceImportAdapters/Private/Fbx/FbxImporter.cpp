#include "PCH.h"

#include "Fbx/FbxImporter.h"

#include "Fbx/FbxGeometryImporter.h"
#include "Fbx/FbxMaterialImporter.h"
#include "Fbx/FbxSceneReader.h"

#include <assimp/Importer.hpp>

#include <format>

static const auto g_fbxImporterLogger = Logging::GetOrCreateLogger("Tools.SourceImportAdapters.Fbx");

bool FbxImporter::SupportsExtension(std::wstring_view extension) const noexcept
{
	return extension == L".fbx";
}

SourceImportResult FbxImporter::Import(const std::filesystem::path& filePath) const
{
	SourceImportResult result;
	result.scene.importerType = SourceImporterType::Fbx;
	result.scene.sourcePath = filePath;

	Assimp::Importer importer;
	const aiScene* scene = nullptr;
	if (!FbxSceneReader::LoadScene(filePath, importer, scene, result))
	{
		return result;
	}

	result.scene.materials.reserve(scene->mNumMaterials);
	result.ReserveMeshes(FbxGeometryImporter::CountImportedMeshInstances(*scene->mRootNode));

	FbxSceneReader::CollectSceneWarnings(*scene, result);
	FbxMaterialImporter::ImportMaterials(*scene, filePath.parent_path(), result);
	FbxGeometryImporter::ImportGeometry(*scene, result);

	if (result.scene.meshes.empty())
	{
		SPDLOG_LOGGER_ERROR(g_fbxImporterLogger, "{}", std::format("FbxImporter: No supported static meshes found in '{}'", filePath.string()));
		return result;
	}

	result.succeeded = true;

	SPDLOG_LOGGER_INFO(
	    g_fbxImporterLogger,
	    "{}",
	    std::format(
	        "FbxImporter: Loaded '{}' - {} meshes, {} materials",
	        filePath.filename().string(),
	        result.scene.meshes.size(),
	        result.scene.materials.size()));

	return result;
}


