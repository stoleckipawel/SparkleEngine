#include "PCH.h"

#include "SceneData/RenderMeshSnapshotAdapter.h"

namespace RenderMeshSnapshotAdapter
{
	RenderMeshKind ToRenderMeshKind(SceneMeshKind meshKind) noexcept
	{
		return meshKind == SceneMeshKind::Skeletal ? RenderMeshKind::Skeletal : RenderMeshKind::Static;
	}

	RenderMeshInstanceGroupKind ToRenderMeshInstanceGroupKind(SceneMeshInstanceGroupKind groupKind) noexcept
	{
		switch (groupKind)
		{
			case SceneMeshInstanceGroupKind::SharedMeshReference:
				return RenderMeshInstanceGroupKind::SharedMeshReference;
			case SceneMeshInstanceGroupKind::AuthoredInstanceGroup:
				return RenderMeshInstanceGroupKind::AuthoredInstanceGroup;
			case SceneMeshInstanceGroupKind::None:
			default:
				return RenderMeshInstanceGroupKind::None;
		}
	}

	RenderMeshInstanceGroupIndex ToRenderMeshInstanceGroupIndex(SceneMeshInstanceGroupIndex groupIndex) noexcept
	{
		return groupIndex == kInvalidSceneMeshInstanceGroupIndex ? kInvalidRenderMeshInstanceGroupIndex : groupIndex;
	}

	std::vector<RenderMeshInstanceGroup> BuildRenderMeshInstanceGroups(const MeshSnapshot& meshSnapshot)
	{
		std::vector<RenderMeshInstanceGroup> renderGroups;
		renderGroups.reserve(meshSnapshot.meshInstanceGroups.size());
		for (const MeshInstanceGroupSnapshot& group : meshSnapshot.meshInstanceGroups)
		{
			renderGroups.push_back(
			    RenderMeshInstanceGroup{
			        .groupKind = ToRenderMeshInstanceGroupKind(group.groupKind),
			        .instanceCount = group.instanceCount});
		}
		return renderGroups;
	}
}
