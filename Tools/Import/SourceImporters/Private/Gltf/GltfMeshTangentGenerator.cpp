#include "PCH.h"

#include "Gltf/GltfMeshTangentGenerator.h"

#include "Gltf/GltfTangentFrameSetGenerator.h"
#include "Gltf/GltfTangentVertexRemapper.h"

void GltfMeshTangentGenerator::GenerateTangents(ImportedMeshGeometry& geometry)
{
	GltfGeneratedTangentFrameSet frames = GltfTangentFrameSetGenerator::Generate(geometry);
	GltfTangentVertexRemapper::Apply(geometry, frames);
}
