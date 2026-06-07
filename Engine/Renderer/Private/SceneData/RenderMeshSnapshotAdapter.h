#pragma once

#include "Renderer/Public/SceneData/RenderMeshClassification.h"
#include "Scene/Meshes/MeshSnapshot.h"

#include <vector>

namespace RenderMeshSnapshotAdapter
{
	RenderMeshKind ToRenderMeshKind(SceneMeshKind meshKind) noexcept;
	RenderMeshInstanceGroupKind ToRenderMeshInstanceGroupKind(SceneMeshInstanceGroupKind groupKind) noexcept;
	RenderMeshInstanceGroupIndex ToRenderMeshInstanceGroupIndex(SceneMeshInstanceGroupIndex groupIndex) noexcept;
	std::vector<RenderMeshInstanceGroup> BuildRenderMeshInstanceGroups(const MeshSnapshot& meshSnapshot);
}
