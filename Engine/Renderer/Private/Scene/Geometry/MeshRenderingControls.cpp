#include "PCH.h"
#include "Scene/Geometry/MeshRenderingControls.h"

#include "Core/Public/Console/CVar.h"

static ConsoleVariable<bool> CVarStaticMeshes("r.Meshes.Static", true, "Render static meshes in raster and ray-traced scene geometry.");
static ConsoleVariable<bool> CVarSkinnedMeshes("r.Meshes.Skinned", true, "Render skinned meshes in raster and ray-traced scene geometry.");

bool IsMeshRenderingEnabled(SceneMeshKind kind) noexcept
{
	switch (kind)
	{
		case SceneMeshKind::Static:
			return CVarStaticMeshes.Get();
		case SceneMeshKind::Skeletal:
			return CVarSkinnedMeshes.Get();
	}
	return false;
}
