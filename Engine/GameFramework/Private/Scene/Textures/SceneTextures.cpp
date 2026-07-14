#include "PCH.h"

#include "Scene/Textures/SceneTextures.h"

#include "Core/Public/FileSystemUtils.h"

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

void SceneTextures::AppendTexturePaths(std::span<const std::filesystem::path> texturePaths)
{
	Filesystem::AppendNormalizedAssetPaths(texturePaths, AssetType::Texture, m_texturePaths);
}

TextureSnapshot SceneTextures::CaptureSnapshot() const
{
	TextureSnapshot snapshot;
	snapshot.texturePaths = m_texturePaths;
	return snapshot;
}

TextureSnapshot SceneTextures::CaptureSnapshot(std::span<const std::filesystem::path> additionalTexturePaths) const
{
	TextureSnapshot snapshot = CaptureSnapshot();
	Filesystem::AppendNormalizedAssetPaths(additionalTexturePaths, AssetType::Texture, snapshot.texturePaths);
	return snapshot;
}
