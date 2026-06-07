#pragma once

#include "Meshes/GPUMesh.h"

class Mesh;

namespace GPUMeshUploadDescBuilder
{
	GPUMeshUploadDesc Build(const Mesh& cpuMesh);
}
