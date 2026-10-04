#include "PCH.h"

#include "Assimp/AssimpSceneReader.h"

#include "Core/Public/Diagnostics/Error.h"

#include <assimp/config.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include <cmath>
#include <format>

constexpr unsigned int AssimpSceneReader::GetPostProcessFlags() noexcept
{
	return aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace | aiProcess_SortByPType
	    | aiProcess_ValidateDataStructure | aiProcess_ImproveCacheLocality | aiProcess_GlobalScale | aiProcess_ConvertToLeftHanded;
}

void AssimpSceneReader::ValidateInputPath(const std::filesystem::path& filePath)
{
	if (!std::filesystem::exists(filePath))
	{
		throw Diagnostics::Error(std::format("Assimp source file does not exist: '{}'.", filePath.string()));
	}
}

const aiScene& AssimpSceneReader::LoadScene(const std::filesystem::path& filePath, Assimp::Importer& importer)
{
	ValidateInputPath(filePath);

	if (filePath.extension() == ".fbx")
	{
		importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
	}
	const aiScene* scene = importer.ReadFile(filePath.string(), GetPostProcessFlags());
	if (scene == nullptr || scene->mRootNode == nullptr)
	{
		throw Diagnostics::Error(std::format("Cannot parse Assimp source '{}' ({}).", filePath.string(), importer.GetErrorString()));
	}
	return *scene;
}

float AssimpSceneReader::GetFbxMetersPerSourceUnit(const Assimp::Importer& importer)
{
	const float metersPerSourceUnit = importer.GetPropertyFloat(AI_CONFIG_APP_SCALE_KEY, 0.0f);
	if (!std::isfinite(metersPerSourceUnit) || metersPerSourceUnit <= 0.0f)
	{
		throw Diagnostics::Error("FBX source does not provide a valid linear-unit conversion to metres.");
	}
	return metersPerSourceUnit;
}
