#include "PCH.h"

#include "Assets/Cooked/CookedTextureAssetUtils.h"

#include "Core/Public/FileSystemUtils.h"
#include "RHI/Public/D3D12/Textures/CookedTextureAsset.h"

#include <format>

namespace Engine::Assets
{
	std::filesystem::path BuildCookedTextureAssetPath(CookedAssetId textureAssetId)
	{
		return Filesystem::GetProjectAssetsPath() / "Cooked" / "Textures" /
		       std::format("{:016X}{}", textureAssetId, kCookedTextureAssetExtension);
	}
}