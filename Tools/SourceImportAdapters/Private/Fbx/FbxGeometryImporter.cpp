#include "PCH.h"

#include "Fbx/FbxGeometryImporter.h"

#include <DirectXMath.h>

#include <format>

static const auto g_fbxGeometryImporterLogger = Logging::GetOrCreateLogger("Tools.SourceImportAdapters.Fbx");

std::size_t FbxGeometryImporter::CountImportedMeshInstances(const aiNode& node) noexcept
{
	std::size_t meshInstanceCount = node.mNumMeshes;
	for (unsigned int childIndex = 0; childIndex < node.mNumChildren; ++childIndex)
	{
		meshInstanceCount += CountImportedMeshInstances(*node.mChildren[childIndex]);
	}

	return meshInstanceCount;
}

void FbxGeometryImporter::ImportGeometry(const aiScene& scene, SourceImportResult& result)
{
	ExtractNodeMeshes(scene, *scene.mRootNode, aiMatrix4x4(), result);
}

void FbxGeometryImporter::ExtractNodeMeshes(
	const aiScene& scene,
	const aiNode& node,
	const aiMatrix4x4& parentTransform,
	SourceImportResult& result)
{
	const aiMatrix4x4 worldTransform = parentTransform * node.mTransformation;

	for (unsigned int meshReferenceIndex = 0; meshReferenceIndex < node.mNumMeshes; ++meshReferenceIndex)
	{
		const unsigned int sceneMeshIndex = node.mMeshes[meshReferenceIndex];
		if (sceneMeshIndex >= scene.mNumMeshes)
		{
			SPDLOG_LOGGER_WARN(
			    g_fbxGeometryImporterLogger,
			    "{}",
			    std::format("FbxImporter: Node '{}' references invalid mesh index {}", GetNodeName(node), sceneMeshIndex));
			continue;
		}

		AppendMeshInstance(node, *scene.mMeshes[sceneMeshIndex], worldTransform, result);
	}

	for (unsigned int childIndex = 0; childIndex < node.mNumChildren; ++childIndex)
	{
		ExtractNodeMeshes(scene, *node.mChildren[childIndex], worldTransform, result);
	}
}

void FbxGeometryImporter::AppendMeshInstance(
	const aiNode& node,
	const aiMesh& mesh,
	const aiMatrix4x4& worldTransform,
	SourceImportResult& result)
{
	MeshData meshData = ExtractMeshGeometry(mesh, node, result);
	if (!meshData.IsValid())
	{
		return;
	}

	SourceImportResult::MeshEntry meshEntry;
	meshEntry.geometry = std::move(meshData);
	meshEntry.displayName = BuildMeshDisplayName(node, mesh);
	meshEntry.transform = ConvertTransform(worldTransform);
	meshEntry.material = ResolveMaterialHandle(mesh, result);
	result.meshes.push_back(std::move(meshEntry));
}

MeshData FbxGeometryImporter::ExtractMeshGeometry(const aiMesh& mesh, const aiNode& node, SourceImportResult& result)
{
	if (!mesh.HasPositions())
	{
		SPDLOG_LOGGER_WARN(
		    g_fbxGeometryImporterLogger,
		    "{}",
		    std::format(
		        "FbxImporter: Skipping mesh '{}' on node '{}' because it has no vertex positions",
		        GetMeshName(mesh),
		        GetNodeName(node)));
		return {};
	}

	if (mesh.HasBones())
	{
		SPDLOG_LOGGER_WARN(
		    g_fbxGeometryImporterLogger,
		    "{}",
		    std::format("FbxImporter: Mesh '{}' contains bones and will be imported as static geometry only", GetMeshName(mesh)));
	}

	if (mesh.mNumAnimMeshes > 0)
	{
		SPDLOG_LOGGER_WARN(
		    g_fbxGeometryImporterLogger,
		    "{}",
		    std::format("FbxImporter: Mesh '{}' contains morph targets which will be ignored", GetMeshName(mesh)));
	}

	MeshData meshData;
	meshData.Reserve(mesh.mNumVertices, mesh.mNumFaces * 3);
	meshData.vertices.resize(mesh.mNumVertices);
	PopulateVertices(mesh, meshData);
	AppendTriangleIndices(mesh, meshData, result);

	if (!meshData.IsValid())
	{
		SPDLOG_LOGGER_WARN(
		    g_fbxGeometryImporterLogger,
		    "{}",
		    std::format("FbxImporter: Mesh '{}' did not produce valid triangle geometry", GetMeshName(mesh)));
	}

	return meshData;
}

void FbxGeometryImporter::PopulateVertices(const aiMesh& mesh, MeshData& meshData)
{
	for (unsigned int vertexIndex = 0; vertexIndex < mesh.mNumVertices; ++vertexIndex)
	{
		VertexData& vertex = meshData.vertices[vertexIndex];
		vertex.position = DirectX::XMFLOAT3(mesh.mVertices[vertexIndex].x, mesh.mVertices[vertexIndex].y, mesh.mVertices[vertexIndex].z);

		if (mesh.HasNormals())
		{
			vertex.normal = DirectX::XMFLOAT3(mesh.mNormals[vertexIndex].x, mesh.mNormals[vertexIndex].y, mesh.mNormals[vertexIndex].z);
		}

		if (mesh.HasTextureCoords(0))
		{
			vertex.uv = DirectX::XMFLOAT2(mesh.mTextureCoords[0][vertexIndex].x, mesh.mTextureCoords[0][vertexIndex].y);
		}

		if (mesh.HasTangentsAndBitangents())
		{
			vertex.tangent = DirectX::XMFLOAT4(mesh.mTangents[vertexIndex].x, mesh.mTangents[vertexIndex].y, mesh.mTangents[vertexIndex].z, 1.0f);
		}

		if (mesh.HasVertexColors(0))
		{
			vertex.color = DirectX::XMFLOAT4(
			    mesh.mColors[0][vertexIndex].r,
			    mesh.mColors[0][vertexIndex].g,
			    mesh.mColors[0][vertexIndex].b,
			    mesh.mColors[0][vertexIndex].a);
		}
	}
}

void FbxGeometryImporter::AppendTriangleIndices(const aiMesh& mesh, MeshData& meshData, SourceImportResult& result)
{
	for (unsigned int faceIndex = 0; faceIndex < mesh.mNumFaces; ++faceIndex)
	{
		const aiFace& face = mesh.mFaces[faceIndex];
		if (face.mNumIndices != 3)
		{
			SPDLOG_LOGGER_WARN(
			    g_fbxGeometryImporterLogger,
			    "{}",
			    std::format("FbxImporter: Skipping non-triangle face {} in mesh '{}'", faceIndex, GetMeshName(mesh)));
			continue;
		}

		meshData.indices.push_back(face.mIndices[0]);
		meshData.indices.push_back(face.mIndices[1]);
		meshData.indices.push_back(face.mIndices[2]);
	}
}

MaterialHandle FbxGeometryImporter::ResolveMaterialHandle(const aiMesh& mesh, SourceImportResult& result) noexcept
{
	if (result.materials.empty())
	{
		return MaterialHandle::Invalid();
	}

	if (mesh.mMaterialIndex < result.materials.size())
	{
		return MaterialHandle(mesh.mMaterialIndex);
	}

	SPDLOG_LOGGER_WARN(
	    g_fbxGeometryImporterLogger,
	    "{}",
	    std::format(
	        "FbxImporter: '{}' references invalid material index {} and will use the default material",
	        GetMeshName(mesh),
	        mesh.mMaterialIndex));
	return MaterialHandle::Invalid();
}

std::string FbxGeometryImporter::BuildMeshDisplayName(const aiNode& node, const aiMesh& mesh)
{
	const std::string nodeName = GetNodeName(node);
	const std::string meshName = GetMeshName(mesh);
	if (nodeName == meshName || meshName == "<unnamed-mesh>")
	{
		return nodeName;
	}
	if (nodeName == "<unnamed-node>")
	{
		return meshName;
	}
	return std::format("{} / {}", nodeName, meshName);
}

std::string FbxGeometryImporter::GetNodeName(const aiNode& node)
{
	if (node.mName.length > 0)
	{
		return node.mName.C_Str();
	}

	return std::string("<unnamed-node>");
}

std::string FbxGeometryImporter::GetMeshName(const aiMesh& mesh)
{
	if (mesh.mName.length > 0)
	{
		return mesh.mName.C_Str();
	}

	return std::string("<unnamed-mesh>");
}

Transform FbxGeometryImporter::ConvertTransform(const aiMatrix4x4& matrix) noexcept
{
	return Transform(DirectX::XMMATRIX(
	    matrix.a1,
	    matrix.a2,
	    matrix.a3,
	    matrix.a4,
	    matrix.b1,
	    matrix.b2,
	    matrix.b3,
	    matrix.b4,
	    matrix.c1,
	    matrix.c2,
	    matrix.c3,
	    matrix.c4,
	    matrix.d1,
	    matrix.d2,
	    matrix.d3,
	    matrix.d4));
}


