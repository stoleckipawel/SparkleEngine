#include "CookedAssetFormatBuilder.h"

#include "FileSystemUtils.h"
#include "Hash/HashUtils.h"

#include <format>

CookedAssetPackageDefinition CookedAssetFormatBuilder::BuildFromSceneImportResult(const SceneImportResult& result)
{
	return BuildFromSceneImportResult(result, CookedAssetBuildOptions{});
}

CookedAssetPackageDefinition CookedAssetFormatBuilder::BuildFromSceneImportResult(
	const SceneImportResult& result,
	const CookedAssetBuildOptions& options)
{
	if (!result.IsValid())
	{
		return {};
	}

	const CookedAssetBuildOptions resolvedOptions = ResolveBuildOptions(result, &options);
	return BuildPackage(result, resolvedOptions);
}

CookedAssetBuildOptions CookedAssetFormatBuilder::ResolveBuildOptions(
	const SceneImportResult& result,
	const CookedAssetBuildOptions* options)
{
	CookedAssetBuildOptions resolvedOptions = options != nullptr ? *options : CookedAssetBuildOptions{};
	resolvedOptions.assetStem = ResolveAssetStem(result, options);
	resolvedOptions.meshAssetRelativePath = ResolveRelativePath(
	    resolvedOptions.assetStem,
	    resolvedOptions.meshAssetRelativePath,
	    ".smesh");
	resolvedOptions.materialAssetRelativePath = ResolveRelativePath(
	    resolvedOptions.assetStem,
	    resolvedOptions.materialAssetRelativePath,
	    ".smat");
	resolvedOptions.textureManifestRelativePath = ResolveRelativePath(
	    resolvedOptions.assetStem,
	    resolvedOptions.textureManifestRelativePath,
	    ".stex");

	if (resolvedOptions.cookedTextureDirectory.empty())
	{
		resolvedOptions.cookedTextureDirectory = "textures";
	}

	return resolvedOptions;
}

CookedAssetPackageDefinition CookedAssetFormatBuilder::BuildPackage(
	const SceneImportResult& result,
	const CookedAssetBuildOptions& options)
{
	CookedAssetPackageDefinition packageDefinition;
	packageDefinition.sceneAsset.header.fileHeader.magic = kCookedSceneAssetMagic;
	packageDefinition.meshAsset.header.fileHeader.magic = kCookedMeshAssetMagic;
	packageDefinition.materialAsset.header.fileHeader.magic = kCookedMaterialAssetMagic;
	packageDefinition.textureManifest.header.fileHeader.magic = kCookedTextureManifestMagic;

	std::vector<TextureRecord> textureRecords;
	BuildMeshAssets(result, options, packageDefinition.meshAsset, packageDefinition.sceneAsset);
	BuildTextureManifest(result, options, textureRecords, packageDefinition.textureManifest);
	BuildMaterialAssets(result, textureRecords, packageDefinition.materialAsset);
	BuildSceneReferences(options, packageDefinition.sceneAsset);
	FinalizeHeaders(packageDefinition);
	return packageDefinition;
}

void CookedAssetFormatBuilder::BuildMeshAssets(
	const SceneImportResult& result,
	const CookedAssetBuildOptions& options,
	CookedMeshAssetDefinition& meshAsset,
	CookedSceneAssetDefinition& sceneAsset)
{
	meshAsset.meshTable.reserve(result.meshes.size());
	meshAsset.vertexBlob.reserve(result.stats.totalVertices);
	meshAsset.indexBlob.reserve(result.stats.totalIndices);
	sceneAsset.meshEntries.reserve(result.meshes.size());

	std::uint32_t vertexOffset = 0;
	std::uint32_t indexOffset = 0;

	for (std::size_t meshIndex = 0; meshIndex < result.meshes.size(); ++meshIndex)
	{
		const MeshData& meshData = result.meshes[meshIndex];
		const std::string meshLabel = BuildSyntheticMeshLabel(options.assetStem, meshIndex);
		const std::uint64_t nameHash = HashMeshLabel(meshLabel);
		const std::uint32_t vertexCount = static_cast<std::uint32_t>(meshData.vertices.size());
		const std::uint32_t indexCount = static_cast<std::uint32_t>(meshData.indices.size());

		meshAsset.meshTable.push_back({
		    nameHash,
		    vertexCount,
		    indexCount,
		    vertexOffset,
		    indexOffset});

		meshAsset.vertexBlob.insert(
		    meshAsset.vertexBlob.end(),
		    meshData.vertices.begin(),
		    meshData.vertices.end());
		meshAsset.indexBlob.insert(
		    meshAsset.indexBlob.end(),
		    meshData.indices.begin(),
		    meshData.indices.end());

		CookedSceneMeshEntry sceneMeshEntry;
		sceneMeshEntry.nameHash = nameHash;
		sceneMeshEntry.materialIndex = meshIndex < result.materialOffsets.size() ? result.materialOffsets[meshIndex] : 0u;
		sceneMeshEntry.vertexOffset = vertexOffset;
		sceneMeshEntry.indexOffset = indexOffset;
		sceneMeshEntry.vertexCount = vertexCount;
		sceneMeshEntry.indexCount = indexCount;

		if (meshIndex < result.transforms.size())
		{
			const Transform& transform = result.transforms[meshIndex];
			sceneMeshEntry.translation = transform.GetTranslation();
			sceneMeshEntry.rotationEuler = transform.GetRotationEuler();
			sceneMeshEntry.scale = transform.GetScale();
		}

		sceneAsset.meshEntries.push_back(sceneMeshEntry);

		vertexOffset += vertexCount;
		indexOffset += indexCount;
	}
}

void CookedAssetFormatBuilder::BuildTextureManifest(
	const SceneImportResult& result,
	const CookedAssetBuildOptions& options,
	std::vector<TextureRecord>& textureRecords,
	CookedTextureManifestDefinition& textureManifest)
{
	textureRecords.reserve(result.stats.uniqueTexturePathCount);

	for (const MaterialDesc& material : result.materials)
	{
		if (material.albedoTexture)
		{
			AddOrUpdateTextureRecord(textureRecords, *material.albedoTexture, CookedTextureUsageFlags::Albedo);
		}

		if (material.normalTexture)
		{
			AddOrUpdateTextureRecord(textureRecords, *material.normalTexture, CookedTextureUsageFlags::Normal);
		}

		if (material.metallicRoughnessTexture)
		{
			AddOrUpdateTextureRecord(textureRecords, *material.metallicRoughnessTexture, CookedTextureUsageFlags::MetallicRoughness);
		}

		if (material.occlusionTexture)
		{
			AddOrUpdateTextureRecord(textureRecords, *material.occlusionTexture, CookedTextureUsageFlags::Occlusion);
		}

		if (material.emissiveTexture)
		{
			AddOrUpdateTextureRecord(textureRecords, *material.emissiveTexture, CookedTextureUsageFlags::Emissive);
		}
	}

	textureManifest.textures.reserve(textureRecords.size());
	for (const TextureRecord& textureRecord : textureRecords)
	{
		CookedTextureEntry entry;
		entry.cookedTexturePath = AppendPathString(
		    textureManifest.stringTable,
		    BuildCookedTextureReference(options.cookedTextureDirectory, textureRecord.sourcePath, textureRecord.usageFlags));
		entry.sourceName = AppendString(textureManifest.stringTable, textureRecord.sourcePath.filename().string());
		entry.sourcePathHash = Hash::Fnv1a64(textureRecord.sourcePath.generic_string());
		entry.usageFlags = textureRecord.usageFlags;
		entry.formatHint = DeriveTextureFormatHint(textureRecord.usageFlags);
		textureManifest.textures.push_back(entry);
	}
}

void CookedAssetFormatBuilder::BuildMaterialAssets(
	const SceneImportResult& result,
	const std::vector<TextureRecord>& textureRecords,
	CookedMaterialAssetDefinition& materialAsset)
{
	materialAsset.materials.reserve(result.materials.size());

	for (const MaterialDesc& material : result.materials)
	{
		CookedMaterialEntry entry;
		entry.name = AppendString(materialAsset.stringTable, material.name);
		entry.baseColor = material.baseColor;
		entry.metallic = material.metallic;
		entry.roughness = material.roughness;
		entry.f0 = material.f0;
		entry.emissiveColor = material.emissiveColor;
		entry.alphaMode = static_cast<std::uint32_t>(material.alphaMode);
		entry.alphaCutoff = material.alphaCutoff;
		entry.albedoTextureIndex = ResolveTextureIndex(textureRecords, material.albedoTexture);
		entry.normalTextureIndex = ResolveTextureIndex(textureRecords, material.normalTexture);
		entry.metallicRoughnessTextureIndex = ResolveTextureIndex(textureRecords, material.metallicRoughnessTexture);
		entry.occlusionTextureIndex = ResolveTextureIndex(textureRecords, material.occlusionTexture);
		entry.emissiveTextureIndex = ResolveTextureIndex(textureRecords, material.emissiveTexture);
		materialAsset.materials.push_back(entry);
	}
}

void CookedAssetFormatBuilder::BuildSceneReferences(
	const CookedAssetBuildOptions& options,
	CookedSceneAssetDefinition& sceneAsset)
{
	sceneAsset.references.reserve(3);

	CookedAssetReferenceEntry meshReference;
	meshReference.kind = CookedAssetReferenceKind::MeshData;
	meshReference.relativePath = AppendPathString(sceneAsset.stringTable, options.meshAssetRelativePath);
	sceneAsset.references.push_back(meshReference);

	CookedAssetReferenceEntry materialReference;
	materialReference.kind = CookedAssetReferenceKind::MaterialData;
	materialReference.relativePath = AppendPathString(sceneAsset.stringTable, options.materialAssetRelativePath);
	sceneAsset.references.push_back(materialReference);

	CookedAssetReferenceEntry textureReference;
	textureReference.kind = CookedAssetReferenceKind::TextureManifest;
	textureReference.relativePath = AppendPathString(sceneAsset.stringTable, options.textureManifestRelativePath);
	sceneAsset.references.push_back(textureReference);
}

void CookedAssetFormatBuilder::FinalizeHeaders(CookedAssetPackageDefinition& packageDefinition) noexcept
{
	CookedSceneAssetHeader& sceneHeader = packageDefinition.sceneAsset.header;
	sceneHeader.fileHeader.headerSize = sizeof(CookedSceneAssetHeader);
	sceneHeader.meshCount = static_cast<std::uint32_t>(packageDefinition.sceneAsset.meshEntries.size());
	sceneHeader.materialCount = static_cast<std::uint32_t>(packageDefinition.materialAsset.materials.size());
	sceneHeader.textureCount = static_cast<std::uint32_t>(packageDefinition.textureManifest.textures.size());
	sceneHeader.meshEntryOffset = sizeof(CookedSceneAssetHeader);
	sceneHeader.referenceEntryOffset = sceneHeader.meshEntryOffset +
	    static_cast<std::uint32_t>(packageDefinition.sceneAsset.meshEntries.size() * sizeof(CookedSceneMeshEntry));
	sceneHeader.referenceCount = static_cast<std::uint32_t>(packageDefinition.sceneAsset.references.size());
	sceneHeader.stringTableOffset = sceneHeader.referenceEntryOffset +
	    static_cast<std::uint32_t>(packageDefinition.sceneAsset.references.size() * sizeof(CookedAssetReferenceEntry));

	CookedMeshAssetHeader& meshHeader = packageDefinition.meshAsset.header;
	meshHeader.fileHeader.headerSize = sizeof(CookedMeshAssetHeader);
	meshHeader.meshCount = static_cast<std::uint32_t>(packageDefinition.meshAsset.meshTable.size());
	meshHeader.meshTableOffset = sizeof(CookedMeshAssetHeader);
	meshHeader.vertexDataOffset = meshHeader.meshTableOffset +
	    static_cast<std::uint32_t>(packageDefinition.meshAsset.meshTable.size() * sizeof(CookedMeshEntry));
	meshHeader.indexDataOffset = meshHeader.vertexDataOffset +
	    static_cast<std::uint32_t>(packageDefinition.meshAsset.vertexBlob.size() * sizeof(VertexData));
	meshHeader.totalVertexCount = packageDefinition.meshAsset.vertexBlob.size();
	meshHeader.totalIndexCount = packageDefinition.meshAsset.indexBlob.size();

	CookedMaterialAssetHeader& materialHeader = packageDefinition.materialAsset.header;
	materialHeader.fileHeader.headerSize = sizeof(CookedMaterialAssetHeader);
	materialHeader.materialCount = static_cast<std::uint32_t>(packageDefinition.materialAsset.materials.size());
	materialHeader.materialEntryOffset = sizeof(CookedMaterialAssetHeader);
	materialHeader.stringTableOffset = materialHeader.materialEntryOffset +
	    static_cast<std::uint32_t>(packageDefinition.materialAsset.materials.size() * sizeof(CookedMaterialEntry));

	CookedTextureManifestHeader& textureHeader = packageDefinition.textureManifest.header;
	textureHeader.fileHeader.headerSize = sizeof(CookedTextureManifestHeader);
	textureHeader.textureCount = static_cast<std::uint32_t>(packageDefinition.textureManifest.textures.size());
	textureHeader.textureEntryOffset = sizeof(CookedTextureManifestHeader);
	textureHeader.stringTableOffset = textureHeader.textureEntryOffset +
	    static_cast<std::uint32_t>(packageDefinition.textureManifest.textures.size() * sizeof(CookedTextureEntry));
}

std::string CookedAssetFormatBuilder::ResolveAssetStem(
	const SceneImportResult& result,
	const CookedAssetBuildOptions* options)
{
	if (options != nullptr && !options->assetStem.empty())
	{
		return SanitizeIdentifier(options->assetStem);
	}

	if (!result.stats.sourcePath.empty())
	{
		const std::string sourceStem = result.stats.sourcePath.stem().string();
		if (!sourceStem.empty())
		{
			return SanitizeIdentifier(sourceStem);
		}
	}

	return "scene";
}

std::filesystem::path CookedAssetFormatBuilder::ResolveRelativePath(
	std::string_view assetStem,
	const std::filesystem::path& configuredPath,
	std::string_view extension)
{
	if (!configuredPath.empty())
	{
		return configuredPath;
	}

	return std::filesystem::path(std::format("{}{}", assetStem, extension));
}

std::string CookedAssetFormatBuilder::BuildSyntheticMeshLabel(std::string_view assetStem, std::size_t meshIndex)
{
	return std::format("{}_mesh_{}", assetStem, meshIndex);
}

std::uint64_t CookedAssetFormatBuilder::HashMeshLabel(std::string_view meshLabel) noexcept
{
	return Hash::Fnv1a64(meshLabel);
}

CookedStringRef CookedAssetFormatBuilder::AppendString(std::vector<char>& stringTable, std::string_view value)
{
	CookedStringRef reference;
	if (value.empty())
	{
		return reference;
	}

	reference.offset = static_cast<std::uint32_t>(stringTable.size());
	reference.length = static_cast<std::uint32_t>(value.size());
	stringTable.insert(stringTable.end(), value.begin(), value.end());
	return reference;
}

CookedStringRef CookedAssetFormatBuilder::AppendPathString(std::vector<char>& stringTable, const std::filesystem::path& path)
{
	return AppendString(stringTable, path.generic_string());
}

std::int32_t CookedAssetFormatBuilder::AddOrUpdateTextureRecord(
	std::vector<TextureRecord>& textureRecords,
	const std::filesystem::path& texturePath,
	CookedTextureUsageFlags usageFlags)
{
	const std::filesystem::path normalizedPath = Filesystem::NormalizePath(texturePath);
	for (std::size_t textureIndex = 0; textureIndex < textureRecords.size(); ++textureIndex)
	{
		if (textureRecords[textureIndex].sourcePath == normalizedPath)
		{
			textureRecords[textureIndex].usageFlags |= usageFlags;
			return static_cast<std::int32_t>(textureIndex);
		}
	}

	textureRecords.push_back({normalizedPath, usageFlags});
	return static_cast<std::int32_t>(textureRecords.size() - 1);
}

std::int32_t CookedAssetFormatBuilder::ResolveTextureIndex(
	const std::vector<TextureRecord>& textureRecords,
	const std::optional<std::filesystem::path>& texturePath)
{
	if (!texturePath)
	{
		return kInvalidCookedTextureIndex;
	}

	const std::filesystem::path normalizedPath = Filesystem::NormalizePath(*texturePath);
	for (std::size_t textureIndex = 0; textureIndex < textureRecords.size(); ++textureIndex)
	{
		if (textureRecords[textureIndex].sourcePath == normalizedPath)
		{
			return static_cast<std::int32_t>(textureIndex);
		}
	}

	return kInvalidCookedTextureIndex;
}

CookedTextureFormatHint CookedAssetFormatBuilder::DeriveTextureFormatHint(CookedTextureUsageFlags usageFlags) noexcept
{
	if (HasAnyCookedTextureUsage(usageFlags, CookedTextureUsageFlags::Normal))
	{
		return CookedTextureFormatHint::NormalMap;
	}

	if (HasAnyCookedTextureUsage(usageFlags, CookedTextureUsageFlags::Albedo | CookedTextureUsageFlags::Emissive))
	{
		return CookedTextureFormatHint::ColorSrgb;
	}

	if (HasAnyCookedTextureUsage(
		usageFlags,
		CookedTextureUsageFlags::MetallicRoughness | CookedTextureUsageFlags::Occlusion))
	{
		return CookedTextureFormatHint::DataLinear;
	}

	return CookedTextureFormatHint::Unknown;
}

std::filesystem::path CookedAssetFormatBuilder::BuildCookedTextureReference(
	const std::filesystem::path& cookedTextureDirectory,
	const std::filesystem::path& sourceTexturePath,
	CookedTextureUsageFlags usageFlags)
{
	const std::string sanitizedStem = SanitizeIdentifier(sourceTexturePath.stem().string());
	const std::uint64_t sourceHash = Hash::Fnv1a64(sourceTexturePath.generic_string());
	const std::string usageSuffix = std::format("{:08x}", static_cast<std::uint32_t>(usageFlags));
	return cookedTextureDirectory /
	    std::format(
	        "{}_{}_{:016x}.ktx2",
	        sanitizedStem.empty() ? "texture" : sanitizedStem,
	        usageSuffix,
	        sourceHash);
}

std::string CookedAssetFormatBuilder::SanitizeIdentifier(std::string_view identifier)
{
	std::string sanitized;
	sanitized.reserve(identifier.size());

	for (const char character : identifier)
	{
		if ((character >= 'a' && character <= 'z') ||
		    (character >= 'A' && character <= 'Z') ||
		    (character >= '0' && character <= '9'))
		{
			sanitized.push_back(character);
			continue;
		}

		if (!sanitized.empty() && sanitized.back() != '_')
		{
			sanitized.push_back('_');
		}
	}

	if (!sanitized.empty() && sanitized.back() == '_')
	{
		sanitized.pop_back();
	}

	return sanitized;
}