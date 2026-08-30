#pragma once

#include "Types/ImportedGeometry.h"

class GltfMeshTangentGenerator final
{
public:
	static void GenerateTangents(ImportedMeshGeometry& geometry);
};
