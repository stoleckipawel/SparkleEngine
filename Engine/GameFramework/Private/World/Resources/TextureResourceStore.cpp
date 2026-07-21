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
	const std::size_t previousCount = m_paths.size();
	Filesystem::AppendNormalizedAssetPaths(paths, AssetType::Texture, m_paths);
	if (m_paths.size() != previousCount) ++m_contentRevision;
}

RenderTextureTable TextureResourceStore::CaptureRenderTable() const
{
	return {.Paths = m_paths, .Generation = m_generation};
}

RenderTextureTable TextureResourceStore::CaptureRenderTable(std::span<const std::filesystem::path> additionalPaths) const
{
	RenderTextureTable table = CaptureRenderTable();
	Filesystem::AppendNormalizedAssetPaths(additionalPaths, AssetType::Texture, table.Paths);
	return table;
}
