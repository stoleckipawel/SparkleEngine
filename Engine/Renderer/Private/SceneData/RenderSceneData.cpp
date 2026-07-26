#include "PCH.h"

#include "SceneData/RenderSceneData.h"

ResolvedMaterialTextureTable::operator bool() const noexcept
{
	return static_cast<bool>(Binding) &&
	       DescriptorCount != 0u &&
	       Generation != 0u;
}
