#include "PCH.h"

#include "Scene/Textures/SceneTextures.h"

#include "FileSystemUtils.h"

#include <algorithm>

void SceneTextures::AppendTexturePaths(const std::vector<std::filesystem::path>& texturePaths)
{
	for (const std::filesystem::path& texturePath : texturePaths)
	{
		std::filesystem::path normalizedPath;
		if (auto resolvedPath = Filesystem::ResolveAssetPath(texturePath, AssetType::Texture))
		{
			normalizedPath = Filesystem::NormalizePath(*resolvedPath);
		}
		else
		{
			normalizedPath = Filesystem::NormalizePath(texturePath);
		}

		if (normalizedPath.empty())
		{
			continue;
		}

		if (std::ranges::find(m_texturePaths, normalizedPath) != m_texturePaths.end())
		{
			continue;
		}

		m_texturePaths.push_back(std::move(normalizedPath));
	}
}

TextureSnapshot SceneTextures::CaptureSnapshot() const
{
	TextureSnapshot snapshot;
	snapshot.texturePaths = m_texturePaths;
	return snapshot;
}