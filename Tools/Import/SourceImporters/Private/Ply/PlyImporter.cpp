#include "PCH.h"

#include "Ply/PlyImporter.h"

#include "Assimp/AssimpSceneReader.h"
#include "Assimp/AssimpGeometryImporter.h"
#include "Assimp/AssimpMaterialImporter.h"
#include "Core/Public/Diagnostics/Error.h"

#include <assimp/Importer.hpp>

SourceImportOutput ImportPlyScene(const std::filesystem::path& filePath)
{
	SourceImportOutput output;
	output.provenance.sourcePath = filePath;
	// PLY does not declare physical units; the source-coordinate contract treats one unit as one metre.
	output.provenance.sourceMetersPerUnit = 1.0f;

	Assimp::Importer importer;
	const aiScene& scene = AssimpSceneReader::LoadScene(filePath, importer);
	output.scene.materials.reserve(scene.mNumMaterials);
	output.ReserveMeshPrimitives(scene.mNumMeshes);
	output.ReserveMeshInstances(AssimpGeometryImporter::CountImportedMeshInstances(*scene.mRootNode));
	AssimpMaterialImporter::ImportMaterials(scene, filePath.parent_path(), {}, output);
	AssimpGeometryImporter::ImportGeometry(scene, output);
	if (output.scene.meshPrimitives.empty() || output.scene.meshInstances.empty())
	{
		throw Diagnostics::Error("PLY import produced no triangle geometry.");
	}
	return output;
}
