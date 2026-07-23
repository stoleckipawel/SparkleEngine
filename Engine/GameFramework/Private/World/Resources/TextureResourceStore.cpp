#include "PCH.h"

#include "World/Resources/TextureResourceStore.h"

#include "Core/Public/FileSystemUtils.h"

class TextureResourceStoreOperations final
{
  public:
	static RenderTextureTable BuildRenderTextureTable(
	    std::span<const std::filesystem::path> paths,
	    std::uint32_t generation)
	{
		RenderTextureTable table;
		table.Generation = generation;
		table.Assets.reserve(paths.size());
		for (std::uint32_t index = 0; index < paths.size(); ++index)
			table.Assets.push_back({RenderTextureAssetHandle{index}, paths[index]});
		return table;
	}
};

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
	return TextureResourceStoreOperations::BuildRenderTextureTable(m_paths, m_generation);
}

RenderTextureTable TextureResourceStore::CaptureRenderTable(std::span<const std::filesystem::path> additionalPaths) const
{
	std::vector<std::filesystem::path> paths = m_paths;
	paths.reserve(paths.size() + additionalPaths.size());
	Filesystem::AppendNormalizedAssetPaths(additionalPaths, AssetType::Texture, paths);
	return TextureResourceStoreOperations::BuildRenderTextureTable(paths, m_generation);
}
