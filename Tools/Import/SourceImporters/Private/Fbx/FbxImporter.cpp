#include "PCH.h"

#include "Fbx/FbxImporter.h"

#include "Fbx/FbxGeometryImporter.h"
#include "Fbx/FbxMaterialImporter.h"
#include "Fbx/FbxSceneReader.h"

#include <assimp/Importer.hpp>

#include <format>

static const auto g_fbxImporterLogger = Logging::GetOrCreateLogger("Tools.SourceImporters.Fbx");

std::string_view FbxImporter::GetImporterId() const noexcept
{
	return "FbxImporter";
}

bool FbxImporter::SupportsExtension(std::wstring_view extension) const noexcept
{
	return extension == L".fbx";
}

SourceImportResult FbxImporter::Import(const std::filesystem::path& filePath) const
{
	SourceImportResult result;
	result.report.sourcePath = filePath;
	result.report.importerId = std::string(GetImporterId());
	result.scene.importerName = result.report.importerId;
	result.scene.sourcePath = filePath;

	Assimp::Importer importer;
	const aiScene* scene = nullptr;
	if (!FbxSceneReader::LoadScene(filePath, importer, scene, result))
	{
		return result;
	}

	result.scene.materials.reserve(scene->mNumMaterials);
	const std::size_t importedMeshInstanceCount = FbxGeometryImporter::CountImportedMeshInstances(*scene->mRootNode);
	result.ReserveMeshPrimitives(scene->mNumMeshes);
	result.ReserveMeshInstances(importedMeshInstanceCount);

	FbxSceneReader::CollectSceneWarnings(*scene, result);
	FbxMaterialImporter::ImportMaterials(*scene, filePath.parent_path(), result);
	FbxGeometryImporter::ImportGeometry(*scene, result);

	if (result.scene.meshPrimitives.empty() || result.scene.meshInstances.empty())
	{
		SPDLOG_LOGGER_ERROR(
		    g_fbxImporterLogger,
		    "{}",
		    std::format("FbxImporter: No supported static meshes found in '{}'", filePath.string()));
		return result;
	}

	result.succeeded = true;
	return result;
}


