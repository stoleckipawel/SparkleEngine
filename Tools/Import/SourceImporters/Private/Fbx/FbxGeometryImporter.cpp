#include "PCH.h"

#include "Fbx/FbxGeometryImporter.h"

#include "Diagnostics/FbxImportDiagnosticLog.h"

#include <DirectXMath.h>

#include <format>
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
			FbxImportDiagnosticLog::ReportInvalidMeshIndex(GetNodeName(node), sceneMeshIndex, result);
			continue;
		}

		AppendMeshInstance(node, *scene.mMeshes[sceneMeshIndex], sceneMeshIndex, worldTransform, result);
	}

	for (unsigned int childIndex = 0; childIndex < node.mNumChildren; ++childIndex)
	{
		ExtractNodeMeshes(scene, *node.mChildren[childIndex], worldTransform, result);
	}
}

void FbxGeometryImporter::AppendMeshInstance(
	const aiNode& node,
	const aiMesh& mesh,
	std::uint32_t sourceMeshIndex,
	const aiMatrix4x4& worldTransform,
	SourceImportResult& result)
{
	ImportedMeshPrimitiveIndex importedPrimitiveIndex = FindImportedPrimitiveIndex(result.scene, sourceMeshIndex);
	if (importedPrimitiveIndex == kInvalidImportedMeshPrimitiveIndex)
	{
		ImportedMeshGeometry meshGeometry = ExtractMeshGeometry(mesh, node, result);
		if (!meshGeometry.IsValid())
		{
			return;
		}

		ImportedMeshPrimitive primitiveEntry;
		primitiveEntry.geometry = std::move(meshGeometry);
		primitiveEntry.displayName = BuildMeshDisplayName(node, mesh);
		primitiveEntry.sourceMeshIndex = sourceMeshIndex;
		primitiveEntry.sourcePrimitiveIndex = 0;
		importedPrimitiveIndex = static_cast<ImportedMeshPrimitiveIndex>(result.scene.meshPrimitives.size());
		result.scene.meshPrimitives.push_back(std::move(primitiveEntry));
	}

	ImportedMeshInstance instanceEntry;
	instanceEntry.primitiveIndex = importedPrimitiveIndex;
	instanceEntry.worldTransform = ConvertTransform(worldTransform);
	instanceEntry.materialIndex = ResolveMaterialIndex(mesh, result);
	instanceEntry.sourceNodeName = GetNodeName(node);
	result.scene.meshInstances.push_back(std::move(instanceEntry));
}

ImportedMeshPrimitiveIndex FbxGeometryImporter::FindImportedPrimitiveIndex(const ImportedScene& scene, std::uint32_t sourceMeshIndex) noexcept
{
	for (std::size_t primitiveIndex = 0; primitiveIndex < scene.meshPrimitives.size(); ++primitiveIndex)
	{
		const ImportedMeshPrimitive& primitive = scene.meshPrimitives[primitiveIndex];
		if (primitive.sourceMeshIndex == sourceMeshIndex)
		{
			return static_cast<ImportedMeshPrimitiveIndex>(primitiveIndex);
		}
	}

	return kInvalidImportedMeshPrimitiveIndex;
}

ImportedMeshGeometry FbxGeometryImporter::ExtractMeshGeometry(const aiMesh& mesh, const aiNode& node, SourceImportResult& result)
{
	if (!mesh.HasPositions())
	{
		FbxImportDiagnosticLog::ReportMissingVertexPositions(GetMeshName(mesh), GetNodeName(node), result);
		return {};
	}

	if (mesh.HasBones())
	{
		FbxImportDiagnosticLog::ReportStaticBones(GetMeshName(mesh), result);
	}

	if (mesh.mNumAnimMeshes > 0)
	{
		FbxImportDiagnosticLog::ReportIgnoredMorphTargets(GetMeshName(mesh), result);
	}

	ImportedMeshGeometry meshGeometry;
	meshGeometry.Reserve(mesh.mNumVertices, mesh.mNumFaces * 3);
	meshGeometry.vertices.resize(mesh.mNumVertices);
	PopulateVertices(mesh, meshGeometry);
	AppendTriangleIndices(mesh, meshGeometry, result);

	if (!meshGeometry.IsValid())
	{
		FbxImportDiagnosticLog::ReportInvalidTriangleGeometry(GetMeshName(mesh), result);
	}

	return meshGeometry;
}

void FbxGeometryImporter::PopulateVertices(const aiMesh& mesh, ImportedMeshGeometry& meshGeometry)
{
	for (unsigned int vertexIndex = 0; vertexIndex < mesh.mNumVertices; ++vertexIndex)
	{
		ImportedVertex& vertex = meshGeometry.vertices[vertexIndex];
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

void FbxGeometryImporter::AppendTriangleIndices(const aiMesh& mesh, ImportedMeshGeometry& meshGeometry, SourceImportResult& result)
{
	for (unsigned int faceIndex = 0; faceIndex < mesh.mNumFaces; ++faceIndex)
	{
		const aiFace& face = mesh.mFaces[faceIndex];
		if (face.mNumIndices != 3)
		{
			FbxImportDiagnosticLog::ReportSkippedNonTriangleFace(faceIndex, GetMeshName(mesh), result);
			continue;
		}

		meshGeometry.indices.push_back(face.mIndices[0]);
		meshGeometry.indices.push_back(face.mIndices[1]);
		meshGeometry.indices.push_back(face.mIndices[2]);
	}
}

ImportedMaterialIndex FbxGeometryImporter::ResolveMaterialIndex(const aiMesh& mesh, SourceImportResult& result) noexcept
{
	if (result.scene.materials.empty())
	{
		return kInvalidImportedMaterialIndex;
	}

	if (mesh.mMaterialIndex < result.scene.materials.size())
	{
		return static_cast<ImportedMaterialIndex>(mesh.mMaterialIndex);
	}

	FbxImportDiagnosticLog::ReportInvalidMaterialIndex(GetMeshName(mesh), mesh.mMaterialIndex, result);
	return kInvalidImportedMaterialIndex;
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

DirectX::XMFLOAT4X4 FbxGeometryImporter::ConvertTransform(const aiMatrix4x4& matrix) noexcept
{
	DirectX::XMFLOAT4X4 transform{};
	DirectX::XMStoreFloat4x4(
	    &transform,
	    DirectX::XMMATRIX(
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
	return transform;
}


