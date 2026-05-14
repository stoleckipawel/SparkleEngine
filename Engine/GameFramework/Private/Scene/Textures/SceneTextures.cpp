#include "PCH.h"

#include "Scene/Textures/SceneTextures.h"

#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Paths/PathUtils.h"

void SceneTextures::AppendMaterialTextureReferences(const std::vector<MaterialDesc>& materialDescs)
{
	std::vector<std::filesystem::path> referencedTexturePaths;
	std::size_t textureReferenceCount = 0;
	for (const MaterialDesc& materialDesc : materialDescs)
	{
		textureReferenceCount += materialDesc.textureReferences.size();
	}
	referencedTexturePaths.reserve(textureReferenceCount);

	for (const MaterialDesc& materialDesc : materialDescs)
	{
		for (const Assets::CookedTextureReference& textureReference : materialDesc.textureReferences)
		{
			if (textureReference.IsValid())
			{
				referencedTexturePaths.emplace_back(textureReference.texturePath);
			}
		}
	}

	AppendTexturePaths(referencedTexturePaths);
}

void SceneTextures::AppendTexturePaths(const std::vector<std::filesystem::path>& texturePaths)
{
	for (const std::filesystem::path& texturePath : texturePaths)
	{
		std::filesystem::path normalizedPath;
		if (auto resolvedPath = Filesystem::ResolveAssetPath(texturePath, AssetType::Texture))
		{
			normalizedPath = Paths::Normalize(*resolvedPath);
		}
		else
		{
			normalizedPath = Paths::Normalize(texturePath);
		}

		if (normalizedPath.empty())
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