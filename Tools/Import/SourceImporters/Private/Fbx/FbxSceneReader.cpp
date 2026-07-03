#include "PCH.h"

#include "Fbx/FbxSceneReader.h"

#include <assimp/config.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include <format>

static const auto g_fbxSceneReaderLogger = Logging::GetOrCreateLogger("Tools.SourceImporters.Fbx");

constexpr unsigned int FbxSceneReader::GetPostProcessFlags() noexcept
{
	return aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace |
	       aiProcess_SortByPType | aiProcess_ValidateDataStructure | aiProcess_ImproveCacheLocality | aiProcess_ConvertToLeftHanded;
}

void FbxSceneReader::ConfigureImporter(Assimp::Importer& importer)
{
	importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
}

bool FbxSceneReader::ValidateInputPath(const std::filesystem::path& filePath, SourceImportResult& result)
{
	if (std::filesystem::exists(filePath))
	{
		return true;
	}

	(void)result;
	SPDLOG_LOGGER_ERROR(g_fbxSceneReaderLogger, "{}", std::format("FbxImporter: File not found: {}", filePath.string()));
	return false;
}

bool FbxSceneReader::LoadScene(
	const std::filesystem::path& filePath,
	Assimp::Importer& importer,
	const aiScene*& scene,
	SourceImportResult& result)
{
	if (!ValidateInputPath(filePath, result))
	{
		return false;
	}

	ConfigureImporter(importer);
	scene = importer.ReadFile(filePath.string(), GetPostProcessFlags());
    if (scene != nullptr && scene->mRootNode != nullptr)
	{
		return true;
	}

	(void)result;
	SPDLOG_LOGGER_ERROR(
	    g_fbxSceneReaderLogger,
	    "{}",
	    std::format("FbxImporter: Failed to parse '{}' ({})", filePath.string(), importer.GetErrorString()));
	return false;
}

void FbxSceneReader::CollectSceneWarnings(const aiScene& scene, SourceImportResult& result)
{
	(void)result;

	if (scene.HasAnimations())
	{
		SPDLOG_LOGGER_WARN(
		    g_fbxSceneReaderLogger,
		    "{}",
		    std::format("FbxImporter: {} animations are present and will be ignored", scene.mNumAnimations));
	}

	if (scene.HasTextures())
	{
		SPDLOG_LOGGER_WARN(
		    g_fbxSceneReaderLogger,
		    "{}",
		    std::format("FbxImporter: {} embedded textures are present and will be ignored", scene.mNumTextures));
	}

	if (scene.HasCameras())
	{
		SPDLOG_LOGGER_WARN(
		    g_fbxSceneReaderLogger,
		    "{}",
		    std::format("FbxImporter: {} cameras are present and will be ignored", scene.mNumCameras));
	}

	if (scene.HasLights())
	{
		SPDLOG_LOGGER_WARN(
		    g_fbxSceneReaderLogger,
		    "{}",
		    std::format("FbxImporter: {} lights are present and will be ignored", scene.mNumLights));
	}
}


