#include "PCH.h"

#include "Scene/Textures/SceneTextures.h"

#include "FileSystemUtils.h"

void SceneTextures::AppendMaterialTextureReferences(const std::vector<MaterialDesc>& materialDescs)
{
	std::vector<std::filesystem::path> referencedTexturePaths;
	referencedTexturePaths.reserve(materialDescs.size() * 5);

	auto appendOptionalTexturePath = [&referencedTexturePaths](const std::optional<std::filesystem::path>& texturePath)
	{
		if (!texturePath)
		{
			return;
		}

		referencedTexturePaths.push_back(*texturePath);
	};

	for (const MaterialDesc& materialDesc : materialDescs)
	{
		appendOptionalTexturePath(materialDesc.albedoTexture);
		appendOptionalTexturePath(materialDesc.normalTexture);
		appendOptionalTexturePath(materialDesc.metallicRoughnessTexture);
		appendOptionalTexturePath(materialDesc.occlusionTexture);
		appendOptionalTexturePath(materialDesc.emissiveTexture);
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

		m_texturePaths.push_back(std::move(normalizedPath));
	}
}

TextureSnapshot SceneTextures::CaptureSnapshot() const
{
	TextureSnapshot snapshot;
	snapshot.texturePaths = m_texturePaths;
	return snapshot;
}