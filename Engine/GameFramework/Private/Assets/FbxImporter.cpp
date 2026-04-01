#include "PCH.h"

#include "Assets/FbxImporter.h"

#include <format>

SceneImportResult FbxImporter::Load(const std::filesystem::path& filePath)
{
	SceneImportResult result;
	result.errorMessage = std::format(
	    "FbxImporter: FBX import is not implemented yet for '{}'",
	    filePath.string());
	return result;
}