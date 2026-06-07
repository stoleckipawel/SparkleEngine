#pragma once

#include "MeshData.h"

struct StaticMeshData
{
	MeshData geometry;

	bool IsValid() const noexcept { return geometry.IsValid(); }
};
