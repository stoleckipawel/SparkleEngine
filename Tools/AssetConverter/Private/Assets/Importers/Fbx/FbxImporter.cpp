#include "PCH.h"

#include "Assets/Importers/Fbx/FbxImporter.h"

#include "Assets/Importers/Fbx/FbxGeometryImporter.h"
#include "Assets/Importers/Fbx/FbxMaterialImporter.h"
#include "Assets/Importers/Fbx/FbxSceneReader.h"

#include <assimp/Importer.hpp>

#include <format>

static const auto g_fbxImporterLogger = Engine::Logging::GetOrCreateLogger("Tools.AssetConverter.Fbx");

bool FbxImporter::SupportsExtension(std::wstring_view extension) const noexcept
{
	return extension == L".fbx";
}

SceneImportResult FbxImporter::Import(const std::filesystem::path& filePath) const
{
	SceneImportResult result;
	result.importerType = SceneImporterType::Fbx;

	Assimp::Importer importer;
	const aiScene* scene = nullptr;
	if (!FbxSceneReader::LoadScene(filePath, importer, scene, result))
	{
		return result;
	}

	result.materials.reserve(scene->mNumMaterials);
	result.Reserve(FbxGeometryImporter::CountImportedMeshInstances(*scene->mRootNode));

	FbxSceneReader::CollectSceneWarnings(*scene, result);
	FbxMaterialImporter::ImportMaterials(*scene, filePath.parent_path(), result);
	FbxGeometryImporter::ImportGeometry(*scene, result);

	if (result.meshes.empty())
	{
		SPDLOG_LOGGER_ERROR(g_fbxImporterLogger, "{}", std::format("FbxImporter: No supported static meshes found in '{}'", filePath.string()));
		return result;
	}

	result.bSuccess = true;

	SPDLOG_LOGGER_INFO(
	    g_fbxImporterLogger,
	    "{}",
	    std::format(
	        "FbxImporter: Loaded '{}' — {} meshes, {} materials",
	        filePath.filename().string(),
	        result.meshes.size(),
	        result.materials.size()));

	return result;
}