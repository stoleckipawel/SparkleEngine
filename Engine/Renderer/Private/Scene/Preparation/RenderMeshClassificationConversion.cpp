#include "PCH.h"
#include "Scene/Preparation/RenderMeshClassificationConversion.h"

namespace RenderMeshClassificationConversion
{
	RenderMeshKind ToRenderMeshKind(SceneMeshKind kind) noexcept
	{
		return kind == SceneMeshKind::Skeletal ? RenderMeshKind::Skeletal : RenderMeshKind::Static;
	}
	RenderMeshInstanceGroupKind ToRenderMeshInstanceGroupKind(SceneMeshInstanceGroupKind kind) noexcept
	{
		switch (kind)
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
	RenderMeshInstanceGroupIndex ToRenderMeshInstanceGroupIndex(SceneMeshInstanceGroupIndex index) noexcept
	{
		return index == kInvalidSceneMeshInstanceGroupIndex ? kInvalidRenderMeshInstanceGroupIndex : index;
	}
}
