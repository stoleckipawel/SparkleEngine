#include "PCH.h"

#include "Meshes/GPUMeshUploadDescBuilder.h"

#include "Scene/Meshes/Mesh.h"
#include "Scene/Meshes/SkeletalCookedMesh.h"

namespace GPUMeshUploadDescBuilder
{
	GPUMeshUploadDesc Build(const Mesh& cpuMesh)
	{
		GPUMeshUploadDesc uploadDesc{.meshData = cpuMesh.GetMeshData()};
		if (const auto* skeletalMesh = dynamic_cast<const SkeletalCookedMesh*>(&cpuMesh))
		{
			uploadDesc.skinInfluences = skeletalMesh->GetSkeletalMeshData().skinInfluences;
			uploadDesc.morphTargets = &skeletalMesh->GetSkeletalMeshData().morphTargets;
		}
		return uploadDesc;
	}
}
