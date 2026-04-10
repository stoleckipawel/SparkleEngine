#include "PCH.h"

#include "Assets/Importers/Fbx/FbxSceneReader.h"

#include <assimp/config.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include <format>

constexpr unsigned int FbxSceneReader::GetPostProcessFlags() noexcept
{
	return aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace |
	       aiProcess_SortByPType | aiProcess_ValidateDataStructure | aiProcess_ImproveCacheLocality | aiProcess_ConvertToLeftHanded;
}

void FbxSceneReader::ConfigureImporter(Assimp::Importer& importer)
{
	importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
}

bool FbxSceneReader::ValidateInputPath(const std::filesystem::path& filePath, SceneImportResult& result)
{
	if (std::filesystem::exists(filePath))
	{
		return true;
	}

	LOG_ERROR(std::format("FbxImporter: File not found: {}", filePath.string()));
	return false;
}

bool FbxSceneReader::LoadScene(
	const std::filesystem::path& filePath,
	Assimp::Importer& importer,
	const aiScene*& scene,
	SceneImportResult& result)
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

	LOG_ERROR(std::format("FbxImporter: Failed to parse '{}' ({})", filePath.string(), importer.GetErrorString()));
	return false;
}

void FbxSceneReader::CollectSceneWarnings(const aiScene& scene, SceneImportResult& result)
{
	if (scene.HasAnimations())
	{
		LOG_WARNING(std::format("FbxImporter: {} animations are present and will be ignored", scene.mNumAnimations));
	}

	if (scene.HasTextures())
	{
		LOG_WARNING(std::format("FbxImporter: {} embedded textures are present and will be ignored", scene.mNumTextures));
	}

	if (scene.HasCameras())
	{
		LOG_WARNING(std::format("FbxImporter: {} cameras are present and will be ignored", scene.mNumCameras));
	}

	if (scene.HasLights())
	{
		LOG_WARNING(std::format("FbxImporter: {} lights are present and will be ignored", scene.mNumLights));
	}
}