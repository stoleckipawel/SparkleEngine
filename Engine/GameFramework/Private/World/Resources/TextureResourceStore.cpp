#include "PCH.h"

#include "World/Resources/TextureResourceStore.h"

#include "Core/Public/FileSystemUtils.h"

void TextureResourceStore::AppendMaterialReferences(const std::vector<MaterialDesc>& materials)
{
	std::vector<std::filesystem::path> paths;
	for (const MaterialDesc& material : materials)
		for (const Assets::CookedTextureReference& texture : material.textureReferences)
			if (texture.IsValid())
				paths.emplace_back(texture.texturePath);
	AppendPaths(paths);
}

void TextureResourceStore::AppendPaths(std::span<const std::filesystem::path> paths)
{
	Filesystem::AppendNormalizedAssetPaths(paths, AssetType::Texture, m_paths);
}

TextureSnapshot TextureResourceStore::CaptureSnapshot() const
{
	TextureSnapshot snapshot;
	snapshot.texturePaths = m_paths;
	snapshot.generation = m_generation;
	return snapshot;
}

TextureSnapshot TextureResourceStore::CaptureSnapshot(std::span<const std::filesystem::path> additionalPaths) const
{
	TextureSnapshot snapshot = CaptureSnapshot();
	Filesystem::AppendNormalizedAssetPaths(additionalPaths, AssetType::Texture, snapshot.texturePaths);
	return snapshot;
}
