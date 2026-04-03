#include "CookedAssetWriter.h"
#include "SceneImporter.h"

#include "Log.h"

#include <filesystem>
#include <format>

int main(int argc, char** argv)
{
	if (argc < 3)
	{
		LOG_ERROR("Usage: SparkleAssetConverter <input_path> <output_dir>");
		return 1;
	}

	const std::filesystem::path inputPath = argv[1];
	const std::filesystem::path outputDirectory = argv[2];

	LOG_INFO(std::format(
	    "SparkleAssetConverter: Importing '{}' into '{}'",
	    inputPath.string(),
	    outputDirectory.string()));

	SceneImportResult result = SceneImporter::Load(inputPath);
	if (!result.bSuccess)
	{
		LOG_ERROR(result.errorMessage.empty() ? std::string("SparkleAssetConverter: Import failed") : result.errorMessage);
		return 1;
	}

	for (const SceneImportWarning& warning : result.warnings)
	{
		LOG_WARNING(warning.message);
	}

	const CookedAssetWriteResult writeResult = CookedAssetWriter::Write(result, outputDirectory);
	if (!writeResult.bSuccess)
	{
		LOG_ERROR(writeResult.errorMessage);
		return 1;
	}

	LOG_INFO(std::format(
	    "SparkleAssetConverter: Wrote '{}' ({} meshes, {} materials, {} textures)",
	    writeResult.sceneAssetPath.string(),
	    result.GetMeshCount(),
	    result.GetMaterialCount(),
	    result.stats.uniqueTexturePathCount));

	return 0;
}
