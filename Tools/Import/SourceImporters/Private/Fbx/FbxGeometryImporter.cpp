#include "PCH.h"

#include "Fbx/FbxGeometryImporter.h"
#include "Fbx/FbxNodeTransformConverter.h"
#include "Fbx/FbxSkinImporter.h"
#include "Core/Public/Diagnostics/Error.h"

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

void FbxGeometryImporter::ImportGeometry(const aiScene& scene, SourceImportOutput& output)
{
	std::uint32_t nextNodeIndex = 0;
	ExtractNodeMeshes(scene, *scene.mRootNode, aiMatrix4x4(), nextNodeIndex, output);
}

void FbxGeometryImporter::ExtractNodeMeshes(
    const aiScene& scene,
    const aiNode& node,
    const aiMatrix4x4& parentTransform,
    std::uint32_t& nextNodeIndex,
    SourceImportOutput& output)
{
	const std::uint32_t sourceNodeIndex = nextNodeIndex++;
	const aiMatrix4x4 worldTransform = parentTransform * node.mTransformation;

	for (unsigned int meshReferenceIndex = 0; meshReferenceIndex < node.mNumMeshes; ++meshReferenceIndex)
	{
		const unsigned int sceneMeshIndex = node.mMeshes[meshReferenceIndex];
		if (sceneMeshIndex >= scene.mNumMeshes)
		{
			throw Diagnostics::Error(std::format("FBX node '{}' references unknown mesh index {}.", GetNodeName(node), sceneMeshIndex));
		}

		AppendMeshInstance(scene, node, *scene.mMeshes[sceneMeshIndex], sceneMeshIndex, sourceNodeIndex, worldTransform, output);
	}

	for (unsigned int childIndex = 0; childIndex < node.mNumChildren; ++childIndex)
	{
		ExtractNodeMeshes(scene, *node.mChildren[childIndex], worldTransform, nextNodeIndex, output);
	}
}

void FbxGeometryImporter::AppendMeshInstance(
    const aiScene& scene,
    const aiNode& node,
    const aiMesh& mesh,
    std::uint32_t sourceMeshIndex,
    std::uint32_t sourceNodeIndex,
    const aiMatrix4x4& worldTransform,
    SourceImportOutput& output)
{
	// aiProcess_SortByPType separates publisher-authored helper curves and points
	// from renderable geometry. They carry no triangles for Sparkle to rasterize,
	// so omit those meshes without weakening triangle validation below.
	if ((mesh.mPrimitiveTypes & aiPrimitiveType_TRIANGLE) == 0)
	{
		return;
	}

	const ImportedSkeletonIndex skeletonIndex = FbxSkinImporter::ImportSkeleton(scene, node, mesh, sourceMeshIndex, output);
	if (mesh.HasBones() && skeletonIndex == kInvalidImportedSkeletonIndex)
	{
		throw Diagnostics::Error(std::format("FBX mesh '{}' has bones but no imported skeleton.", GetMeshName(mesh)));
	}
	const ImportedSkeleton* skeleton = skeletonIndex < output.scene.skeletons.size() ? &output.scene.skeletons[skeletonIndex] : nullptr;

	ImportedMeshPrimitiveIndex importedPrimitiveIndex = FindImportedPrimitiveIndex(output.scene, sourceMeshIndex);
	if (importedPrimitiveIndex == kInvalidImportedMeshPrimitiveIndex)
	{
		ImportedMeshGeometry meshGeometry = ExtractMeshGeometry(mesh, node, skeleton, output);

		ImportedMeshPrimitive primitiveEntry;
		primitiveEntry.geometry = std::move(meshGeometry);
		primitiveEntry.displayName = BuildMeshDisplayName(node, mesh);
		primitiveEntry.sourceMeshIndex = sourceMeshIndex;
		primitiveEntry.sourcePrimitiveIndex = 0;
		importedPrimitiveIndex = static_cast<ImportedMeshPrimitiveIndex>(output.scene.meshPrimitives.size());
		output.scene.meshPrimitives.push_back(std::move(primitiveEntry));
	}

	ImportedMeshInstance instanceEntry;
	instanceEntry.primitiveIndex = importedPrimitiveIndex;
	instanceEntry.worldTransform = FbxNodeTransformConverter::ConvertAssimpTransformToEngine(worldTransform);
	instanceEntry.materialIndex = ResolveMaterialIndex(mesh, output);
	instanceEntry.skeletonIndex = skeletonIndex;
	instanceEntry.sourceNodeIndex = sourceNodeIndex;
	instanceEntry.sourceNodeName = GetNodeName(node);
	output.scene.meshInstances.push_back(std::move(instanceEntry));
}

ImportedMeshPrimitiveIndex FbxGeometryImporter::FindImportedPrimitiveIndex(
    const ImportedScene& scene,
    std::uint32_t sourceMeshIndex) noexcept
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

ImportedMeshGeometry FbxGeometryImporter::ExtractMeshGeometry(
    const aiMesh& mesh,
    const aiNode& node,
    const ImportedSkeleton* skeleton,
    SourceImportOutput& output)
{
	if (!mesh.HasPositions())
	{
		throw Diagnostics::Error(std::format("FBX mesh '{}' on node '{}' has no vertex positions.", GetMeshName(mesh), GetNodeName(node)));
	}

	if (mesh.mNumAnimMeshes > 0)
	{
		throw Diagnostics::Error(std::format("FBX mesh '{}' contains unsupported morph targets.", GetMeshName(mesh)));
	}

	ImportedMeshGeometry meshGeometry;
	meshGeometry.Reserve(mesh.mNumVertices, mesh.mNumFaces * 3);
	meshGeometry.vertices.resize(mesh.mNumVertices);
	PopulateVertices(mesh, meshGeometry);
	AppendTriangleIndices(mesh, meshGeometry);
	if (mesh.HasBones() && skeleton == nullptr)
	{
		throw Diagnostics::Error(std::format("FBX mesh '{}' has incomplete skin influences.", GetMeshName(mesh)));
	}
	if (mesh.HasBones())
	{
		FbxSkinImporter::ImportSkinInfluences(mesh, *skeleton, meshGeometry);
	}

	if (!meshGeometry.IsValid())
	{
		throw Diagnostics::Error(std::format("FBX mesh '{}' did not produce complete triangle geometry.", GetMeshName(mesh)));
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
			const aiVector3D& normal = mesh.mNormals[vertexIndex];
			const aiVector3D& tangent = mesh.mTangents[vertexIndex];
			const aiVector3D& bitangent = mesh.mBitangents[vertexIndex];
			const float handedness = (normal ^ tangent) * bitangent < 0.0f ? -1.0f : 1.0f;
			vertex.tangent = DirectX::XMFLOAT4(tangent.x, tangent.y, tangent.z, handedness);
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

void FbxGeometryImporter::AppendTriangleIndices(const aiMesh& mesh, ImportedMeshGeometry& meshGeometry)
{
	for (unsigned int faceIndex = 0; faceIndex < mesh.mNumFaces; ++faceIndex)
	{
		const aiFace& face = mesh.mFaces[faceIndex];
		if (face.mNumIndices != 3)
		{
			throw Diagnostics::Error(std::format("FBX face {} in mesh '{}' is not a triangle.", faceIndex, GetMeshName(mesh)));
		}
		for (unsigned int faceIndexOffset = 0; faceIndexOffset < face.mNumIndices; ++faceIndexOffset)
		{
			if (face.mIndices[faceIndexOffset] >= mesh.mNumVertices)
			{
				throw Diagnostics::Error(
				    std::format(
				        "FBX face {} in mesh '{}' references unknown vertex {}.",
				        faceIndex,
				        GetMeshName(mesh),
				        face.mIndices[faceIndexOffset]));
			}
		}

		meshGeometry.indices.push_back(face.mIndices[0]);
		meshGeometry.indices.push_back(face.mIndices[1]);
		meshGeometry.indices.push_back(face.mIndices[2]);
	}
}

ImportedMaterialIndex FbxGeometryImporter::ResolveMaterialIndex(const aiMesh& mesh, const SourceImportOutput& output)
{
	if (output.scene.materials.empty())
	{
		return kInvalidImportedMaterialIndex;
	}

	if (mesh.mMaterialIndex < output.scene.materials.size())
	{
		return static_cast<ImportedMaterialIndex>(mesh.mMaterialIndex);
	}

	throw Diagnostics::Error(std::format("FBX mesh '{}' references unknown material index {}.", GetMeshName(mesh), mesh.mMaterialIndex));
}

std::string FbxGeometryImporter::BuildMeshDisplayName(const aiNode& node, const aiMesh& mesh)
{
	std::string nodeName = GetNodeName(node);
	std::string meshName = GetMeshName(mesh);
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
