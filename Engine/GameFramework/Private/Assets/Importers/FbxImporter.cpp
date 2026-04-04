#include "PCH.h"

#include "Assets/Importers/FbxImporter.h"
#include "Assets/Import/SceneImportUtilities.h"

#include <assimp/config.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include <format>

using namespace DirectX;

SceneImportResult FbxImporter::Load(const std::filesystem::path& filePath)
{
	SceneImportResult result;

	if (!ValidateInputPath(filePath, result))
	{
		return result;
	}

	Assimp::Importer importer;
	ConfigureImporter(importer);

	const aiScene* scene = nullptr;
	if (!TryReadScene(importer, filePath, scene, result))
	{
		return result;
	}

	PrepareResultStorage(*scene, result);
	CollectSceneWarnings(*scene, result);
	ExtractMaterials(*scene, filePath.parent_path(), result);
	ExtractNodeMeshes(*scene, *scene->mRootNode, aiMatrix4x4(), result);

	if (result.meshes.empty())
	{
		result.errorMessage = std::format(
		    "FbxImporter: No supported static meshes found in '{}'",
		    filePath.string());
		return result;
	}

	result.bSuccess = true;

	LOG_INFO(
	    std::format(
	        "FbxImporter: Loaded '{}' — {} meshes, {} materials",
	        filePath.filename().string(),
	        result.meshes.size(),
	        result.materials.size()));

	return result;
}

constexpr unsigned int FbxImporter::GetPostProcessFlags() noexcept
{
	return aiProcess_Triangulate |
	       aiProcess_JoinIdenticalVertices |
	       aiProcess_GenSmoothNormals |
	       aiProcess_CalcTangentSpace |
	       aiProcess_SortByPType |
	       aiProcess_ValidateDataStructure |
	       aiProcess_ImproveCacheLocality |
	       aiProcess_ConvertToLeftHanded;
}

void FbxImporter::ConfigureImporter(Assimp::Importer& importer)
{
	importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
}

bool FbxImporter::ValidateInputPath(const std::filesystem::path& filePath, SceneImportResult& result)
{
	if (std::filesystem::exists(filePath))
	{
		return true;
	}

	result.errorMessage = std::format("FbxImporter: File not found: {}", filePath.string());
	return false;
}

bool FbxImporter::TryReadScene(
	Assimp::Importer& importer,
	const std::filesystem::path& filePath,
	const aiScene*& scene,
	SceneImportResult& result)
{
	scene = importer.ReadFile(filePath.string(), GetPostProcessFlags());
	if (scene != nullptr && scene->mRootNode != nullptr)
	{
		return true;
	}

	result.errorMessage = std::format(
	    "FbxImporter: Failed to parse '{}' ({})",
	    filePath.string(),
	    importer.GetErrorString());
	return false;
}

void FbxImporter::PrepareResultStorage(const aiScene& scene, SceneImportResult& result)
{
	result.materials.reserve(scene.mNumMaterials);
	result.Reserve(CountNodeMeshInstances(*scene.mRootNode));
}

std::size_t FbxImporter::CountNodeMeshInstances(const aiNode& node) noexcept
{
	std::size_t meshInstanceCount = node.mNumMeshes;
	for (unsigned int childIndex = 0; childIndex < node.mNumChildren; ++childIndex)
	{
		meshInstanceCount += CountNodeMeshInstances(*node.mChildren[childIndex]);
	}

	return meshInstanceCount;
}

void FbxImporter::CollectSceneWarnings(const aiScene& scene, SceneImportResult& result)
{
	if (scene.HasAnimations())
	{
		result.AddWarning(std::format(
		    "FbxImporter: {} animations are present and will be ignored",
		    scene.mNumAnimations));
	}

	if (scene.HasTextures())
	{
		result.AddWarning(std::format(
		    "FbxImporter: {} embedded textures are present and will be ignored",
		    scene.mNumTextures));
	}

	if (scene.HasCameras())
	{
		result.AddWarning(std::format(
		    "FbxImporter: {} cameras are present and will be ignored",
		    scene.mNumCameras));
	}

	if (scene.HasLights())
	{
		result.AddWarning(std::format(
		    "FbxImporter: {} lights are present and will be ignored",
		    scene.mNumLights));
	}
}

void FbxImporter::CollectMaterialWarnings(const aiMaterial& material, std::string_view materialName, SceneImportResult& result)
{
	int shadingModel = 0;
	if (material.Get(AI_MATKEY_SHADING_MODEL, shadingModel) != AI_SUCCESS)
	{
		return;
	}

	const aiShadingMode shadingMode = static_cast<aiShadingMode>(shadingModel);
	if (shadingMode == aiShadingMode_NoShading ||
	    shadingMode == aiShadingMode_Flat ||
	    shadingMode == aiShadingMode_Gouraud ||
	    shadingMode == aiShadingMode_Phong ||
	    shadingMode == aiShadingMode_Blinn ||
	    shadingMode == aiShadingMode_Unlit ||
	    shadingMode == aiShadingMode_PBR_BRDF)
	{
		return;
	}

	result.AddWarning(std::format(
	    "FbxImporter: Material '{}' uses unsupported shading model {} and will be approximated with Sparkle PBR defaults",
	    materialName,
	    shadingModel));
}

void FbxImporter::ExtractMaterials(
	const aiScene& scene,
	const std::filesystem::path& sourceDirectory,
	SceneImportResult& result)
{
	for (unsigned int materialIndex = 0; materialIndex < scene.mNumMaterials; ++materialIndex)
	{
		result.materials.push_back(ExtractMaterial(*scene.mMaterials[materialIndex], materialIndex, sourceDirectory, result));
	}
}

MaterialDesc FbxImporter::ExtractMaterial(
	const aiMaterial& material,
	unsigned int materialIndex,
	const std::filesystem::path& sourceDirectory,
	SceneImportResult& result)
{
	MaterialDesc materialDesc = SceneImportUtilities::CreateMaterialDesc(GetMaterialName(material, materialIndex));
	CollectMaterialWarnings(material, materialDesc.name, result);
	ApplyMaterialProperties(material, materialDesc);
	ApplyTextureMappings(material, sourceDirectory, materialDesc, result);
	return materialDesc;
}

void FbxImporter::ApplyMaterialProperties(const aiMaterial& material, MaterialDesc& materialDesc)
{
	aiColor4D baseColor;
	if (aiGetMaterialColor(&material, AI_MATKEY_BASE_COLOR, &baseColor) == AI_SUCCESS ||
	    aiGetMaterialColor(&material, AI_MATKEY_COLOR_DIFFUSE, &baseColor) == AI_SUCCESS)
	{
		materialDesc.baseColor = XMFLOAT4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);
	}

	ai_real opacity = 1.0f;
	if (material.Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS)
	{
		materialDesc.baseColor.w = static_cast<float>(opacity);
		if (materialDesc.baseColor.w < 1.0f)
		{
			materialDesc.alphaMode = AlphaMode::Blend;
		}
	}

	aiColor4D emissiveColor;
	if (aiGetMaterialColor(&material, AI_MATKEY_COLOR_EMISSIVE, &emissiveColor) == AI_SUCCESS)
	{
		materialDesc.emissiveColor = XMFLOAT3(emissiveColor.r, emissiveColor.g, emissiveColor.b);
	}

	ai_real metallic = 0.0f;
	if (material.Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS)
	{
		materialDesc.metallic = static_cast<float>(metallic);
	}

	ai_real roughness = 0.5f;
	if (material.Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS)
	{
		materialDesc.roughness = static_cast<float>(roughness);
	}
}

void FbxImporter::ApplyTextureMappings(
	const aiMaterial& material,
	const std::filesystem::path& sourceDirectory,
	MaterialDesc& materialDesc,
	SceneImportResult& result)
{
	SceneImportUtilities::SetMaterialTexture(
	    materialDesc,
	    ImportedTextureSemantic::Albedo,
	    ResolveTexturePath(
	        material,
	        sourceDirectory,
	        aiTextureType_BASE_COLOR,
	        "base-color",
	        materialDesc.name,
	        result));

	if (!materialDesc.albedoTexture)
	{
		SceneImportUtilities::SetMaterialTexture(
		    materialDesc,
		    ImportedTextureSemantic::Albedo,
		    ResolveTexturePath(
		        material,
		        sourceDirectory,
		        aiTextureType_DIFFUSE,
		        "diffuse",
		        materialDesc.name,
		        result));
	}

	SceneImportUtilities::SetMaterialTexture(
	    materialDesc,
	    ImportedTextureSemantic::Normal,
	    ResolveTexturePath(
	        material,
	        sourceDirectory,
	        aiTextureType_NORMALS,
	        "normal",
	        materialDesc.name,
	        result));

	if (!materialDesc.normalTexture)
	{
		SceneImportUtilities::SetMaterialTexture(
		    materialDesc,
		    ImportedTextureSemantic::Normal,
		    ResolveTexturePath(
		        material,
		        sourceDirectory,
		        aiTextureType_HEIGHT,
		        "height",
		        materialDesc.name,
		        result));
	}

	const std::optional<std::filesystem::path> specularTexturePath = ResolveTexturePath(
	    material,
	    sourceDirectory,
	    aiTextureType_SPECULAR,
	    "specular",
	    materialDesc.name,
	    result);

	if (specularTexturePath)
	{
		if (!materialDesc.metallicRoughnessTexture)
		{
			SceneImportUtilities::SetMaterialTexture(
			    materialDesc,
			    ImportedTextureSemantic::MetallicRoughness,
			    specularTexturePath);
			materialDesc.metallic = 1.0f;
			materialDesc.roughness = 1.0f;
		}

		if (!materialDesc.occlusionTexture)
		{
			SceneImportUtilities::SetMaterialTexture(
			    materialDesc,
			    ImportedTextureSemantic::Occlusion,
			    specularTexturePath);
		}
	}

	SceneImportUtilities::SetMaterialTexture(
	    materialDesc,
	    ImportedTextureSemantic::Emissive,
	    ResolveTexturePath(
	        material,
	        sourceDirectory,
	        aiTextureType_EMISSIVE,
	        "emissive",
	        materialDesc.name,
	        result));
}

std::optional<std::filesystem::path> FbxImporter::ResolveTexturePath(
	const aiMaterial& material,
	const std::filesystem::path& sourceDirectory,
	aiTextureType textureType,
	std::string_view slotName,
	const std::string& materialName,
	SceneImportResult& result)
{
	const unsigned int textureCount = material.GetTextureCount(textureType);
	if (textureCount == 0)
	{
		return std::nullopt;
	}

	if (textureCount > 1)
	{
		result.AddWarning(std::format(
		    "FbxImporter: Material '{}' has multiple {} textures and only the first will be used",
		    materialName,
		    slotName));
	}

	aiString texturePath;
	if (material.GetTexture(textureType, 0, &texturePath) != AI_SUCCESS)
	{
		return std::nullopt;
	}

	const std::string texturePathString = texturePath.C_Str();
	if (texturePathString.empty())
	{
		return std::nullopt;
	}

	if (texturePathString[0] == '*')
	{
		result.AddWarning(std::format(
		    "FbxImporter: Material '{}' uses embedded {} texture '{}' which is not supported yet",
		    materialName,
		    slotName,
		    texturePathString));
		return std::nullopt;
	}

	return SceneImportUtilities::NormalizeImportedTexturePath(sourceDirectory, std::filesystem::path(texturePathString));
}

std::string FbxImporter::GetMaterialName(const aiMaterial& material, unsigned int materialIndex)
{
	aiString name;
	if (material.Get(AI_MATKEY_NAME, name) == AI_SUCCESS && name.length > 0)
	{
		return name.C_Str();
	}

	return std::format("FBXMaterial_{}", materialIndex);
}

void FbxImporter::ExtractNodeMeshes(
	const aiScene& scene,
	const aiNode& node,
	const aiMatrix4x4& parentTransform,
	SceneImportResult& result)
{
	const aiMatrix4x4 worldTransform = parentTransform * node.mTransformation;

	for (unsigned int meshReferenceIndex = 0; meshReferenceIndex < node.mNumMeshes; ++meshReferenceIndex)
	{
		const unsigned int sceneMeshIndex = node.mMeshes[meshReferenceIndex];
		if (sceneMeshIndex >= scene.mNumMeshes)
		{
			result.AddWarning(std::format(
			    "FbxImporter: Node '{}' references invalid mesh index {}",
			    GetNodeName(node),
			    sceneMeshIndex));
			continue;
		}

		AppendMeshInstance(node, *scene.mMeshes[sceneMeshIndex], worldTransform, result);
	}

	for (unsigned int childIndex = 0; childIndex < node.mNumChildren; ++childIndex)
	{
		ExtractNodeMeshes(scene, *node.mChildren[childIndex], worldTransform, result);
	}
}

void FbxImporter::AppendMeshInstance(
	const aiNode& node,
	const aiMesh& mesh,
	const aiMatrix4x4& worldTransform,
	SceneImportResult& result)
{
	MeshData meshData = ExtractMeshGeometry(mesh, node, result);
	if (!meshData.IsValid())
	{
		return;
	}

	result.materialOffsets.push_back(ResolveMaterialOffset(mesh, result));
	result.transforms.push_back(ConvertTransform(worldTransform));
	result.meshes.push_back(std::move(meshData));
}

MeshData FbxImporter::ExtractMeshGeometry(const aiMesh& mesh, const aiNode& node, SceneImportResult& result)
{
	if (!mesh.HasPositions())
	{
		result.AddWarning(std::format(
		    "FbxImporter: Skipping mesh '{}' on node '{}' because it has no vertex positions",
		    GetMeshName(mesh),
		    GetNodeName(node)));
		return {};
	}

	if (mesh.HasBones())
	{
		result.AddWarning(std::format(
		    "FbxImporter: Mesh '{}' contains bones and will be imported as static geometry only",
		    GetMeshName(mesh)));
	}

	if (mesh.mNumAnimMeshes > 0)
	{
		result.AddWarning(std::format(
		    "FbxImporter: Mesh '{}' contains morph targets which will be ignored",
		    GetMeshName(mesh)));
	}

	MeshData meshData;
	meshData.Reserve(mesh.mNumVertices, mesh.mNumFaces * 3);
	meshData.vertices.resize(mesh.mNumVertices);
	PopulateVertices(mesh, meshData);
	AppendTriangleIndices(mesh, meshData, result);

	if (!meshData.IsValid())
	{
		result.AddWarning(std::format(
		    "FbxImporter: Mesh '{}' did not produce valid triangle geometry",
		    GetMeshName(mesh)));
	}

	return meshData;
}

void FbxImporter::PopulateVertices(const aiMesh& mesh, MeshData& meshData)
{
	for (unsigned int vertexIndex = 0; vertexIndex < mesh.mNumVertices; ++vertexIndex)
	{
		VertexData& vertex = meshData.vertices[vertexIndex];
		vertex.position = XMFLOAT3(
		    mesh.mVertices[vertexIndex].x,
		    mesh.mVertices[vertexIndex].y,
		    mesh.mVertices[vertexIndex].z);

		if (mesh.HasNormals())
		{
			vertex.normal = XMFLOAT3(
			    mesh.mNormals[vertexIndex].x,
			    mesh.mNormals[vertexIndex].y,
			    mesh.mNormals[vertexIndex].z);
		}

		if (mesh.HasTextureCoords(0))
		{
			vertex.uv = XMFLOAT2(mesh.mTextureCoords[0][vertexIndex].x, mesh.mTextureCoords[0][vertexIndex].y);
		}

		if (mesh.HasTangentsAndBitangents())
		{
			vertex.tangent = XMFLOAT4(
			    mesh.mTangents[vertexIndex].x,
			    mesh.mTangents[vertexIndex].y,
			    mesh.mTangents[vertexIndex].z,
			    1.0f);
		}

		if (mesh.HasVertexColors(0))
		{
			vertex.color = XMFLOAT4(
			    mesh.mColors[0][vertexIndex].r,
			    mesh.mColors[0][vertexIndex].g,
			    mesh.mColors[0][vertexIndex].b,
			    mesh.mColors[0][vertexIndex].a);
		}
	}
}

void FbxImporter::AppendTriangleIndices(const aiMesh& mesh, MeshData& meshData, SceneImportResult& result)
{
	for (unsigned int faceIndex = 0; faceIndex < mesh.mNumFaces; ++faceIndex)
	{
		const aiFace& face = mesh.mFaces[faceIndex];
		if (face.mNumIndices != 3)
		{
			result.AddWarning(std::format(
			    "FbxImporter: Skipping non-triangle face {} in mesh '{}'",
			    faceIndex,
			    GetMeshName(mesh)));
			continue;
		}

		meshData.indices.push_back(face.mIndices[0]);
		meshData.indices.push_back(face.mIndices[1]);
		meshData.indices.push_back(face.mIndices[2]);
	}
}

std::uint32_t FbxImporter::ResolveMaterialOffset(const aiMesh& mesh, SceneImportResult& result) noexcept
{
	return SceneImportUtilities::SanitizeMaterialOffset(
	    mesh.mMaterialIndex,
	    result.materials.size(),
	    "FbxImporter",
	    GetMeshName(mesh),
	    result);
}

std::string FbxImporter::GetNodeName(const aiNode& node)
{
	if (node.mName.length > 0)
	{
		return node.mName.C_Str();
	}

	return std::string("<unnamed-node>");
}

std::string FbxImporter::GetMeshName(const aiMesh& mesh)
{
	if (mesh.mName.length > 0)
	{
		return mesh.mName.C_Str();
	}

	return std::string("<unnamed-mesh>");
}

Transform FbxImporter::ConvertTransform(const aiMatrix4x4& matrix) noexcept
{
	return SceneImportUtilities::BuildImportedTransform(XMMATRIX(
	    matrix.a1, matrix.a2, matrix.a3, matrix.a4,
	    matrix.b1, matrix.b2, matrix.b3, matrix.b4,
	    matrix.c1, matrix.c2, matrix.c3, matrix.c4,
	    matrix.d1, matrix.d2, matrix.d3, matrix.d4));
}