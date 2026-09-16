#pragma once

#include "Scene/Geometry/RenderMeshClassification.h"
#include "Scene/Meshes/MeshInstanceGroup.h"
#include "Scene/Meshes/SceneMeshKind.h"

namespace RenderMeshClassificationConversion
{
	RenderMeshKind ToRenderMeshKind(SceneMeshKind meshKind) noexcept;
	RenderMeshInstanceGroupKind ToRenderMeshInstanceGroupKind(SceneMeshInstanceGroupKind groupKind) noexcept;
	RenderMeshInstanceGroupIndex ToRenderMeshInstanceGroupIndex(SceneMeshInstanceGroupIndex groupIndex) noexcept;
}
