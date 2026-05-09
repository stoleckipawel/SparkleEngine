#include "PCH.h"

#include "Cooking/CookedSceneCooker.h"

#include "CookArtifactCache.h"
#include "Cooking/TextureCookRequestBuilder.h"

#include "Core/Public/Files/BinaryStreamWriter.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Assets/SceneAssetRegistry.h"

#include <format>
#include <fstream>
#include <limits>
#include <optional>
#include <unordered_set>

	static constexpr std::uint32_t kAssetConverterCookerVersion = 1;

	static Cook::CookArtifactKey BuildMeshCookArtifactKey(
	    const CookedMeshAssetBuild& meshAsset,
	    const Assets::CookedMeshAssetHeader& header,
	    const std::filesystem::path& outputPath)
	{
		std::uint64_t contentHash = Hash::ContinueFnv1a64Value(Hash::kFnv64OffsetBasis, header);
		contentHash = Hash::ContinueFnv1a64Vector(contentHash, meshAsset.vertices);
		contentHash = Hash::ContinueFnv1a64Vector(contentHash, meshAsset.indices);

		return Cook::CookArtifactKey{
		    .assetType = "Mesh",
		    .assetId = Formatting::FormatHexUInt64(meshAsset.assetId),
		    .cookerName = "AssetConverter",
		    .outputPath = outputPath,
		    .cookedFormatVersion = Assets::kCookedMeshAssetVersion,
		    .cookerVersion = kAssetConverterCookerVersion,
		    .sourceHash = Hash::FinalizeFnv1a64(contentHash),
		    .dependencyHash = 0,
		    .settingsHash = Cook::CookArtifactCache::ComputeSettingsHash("CookedMeshAsset")};
	}

	static Cook::CookArtifactKey BuildMaterialCookArtifactKey(
	    const CookedMaterialAssetBuild& materialAsset,
	    const std::filesystem::path& outputPath)
	{
		std::uint64_t contentHash = Hash::ContinueFnv1a64Value(Hash::kFnv64OffsetBasis, materialAsset.header);
		contentHash = Hash::ContinueFnv1a64(contentHash, materialAsset.name.data(), materialAsset.name.size());
		contentHash = Hash::ContinueFnv1a64Vector(contentHash, materialAsset.textureReferences);

		return Cook::CookArtifactKey{
		    .assetType = "Material",
		    .assetId = Formatting::FormatHexUInt64(materialAsset.assetId),
		    .cookerName = "AssetConverter",
		    .outputPath = outputPath,
		    .cookedFormatVersion = Assets::kCookedMaterialAssetVersion,
		    .cookerVersion = kAssetConverterCookerVersion,
		    .sourceHash = Hash::FinalizeFnv1a64(contentHash),
		    .dependencyHash = 0,
		    .settingsHash = Cook::CookArtifactCache::ComputeSettingsHash("CookedMaterialAsset")};
	}

	static Cook::CookArtifactKey BuildSceneManifestCookArtifactKey(const CookedSceneBuild& build)
	{
		std::uint64_t contentHash = Hash::ContinueFnv1a64Value(Hash::kFnv64OffsetBasis, build.manifestHeader);
		contentHash = Hash::ContinueFnv1a64Vector(contentHash, build.meshAssetReferences);
		contentHash = Hash::ContinueFnv1a64Vector(contentHash, build.materialAssetReferences);
		contentHash = Hash::ContinueFnv1a64Vector(contentHash, build.instances);

		return Cook::CookArtifactKey{
		    .assetType = "SceneManifest",
		    .assetId = build.sceneAssetId,
		    .cookerName = "AssetConverter",
		    .outputPath = build.sceneManifestPath,
		    .cookedFormatVersion = Assets::kCookedSceneManifestVersion,
		    .cookerVersion = kAssetConverterCookerVersion,
		    .sourceHash = Hash::FinalizeFnv1a64(contentHash),
		    .dependencyHash = 0,
		    .settingsHash = Cook::CookArtifactCache::ComputeSettingsHash("CookedSceneManifest")};
	}

	CookedSceneBuild CookedSceneCooker::Cook(const std::filesystem::path& sourceScenePath, const SceneImportResult& importResult) const
	{
		CookedSceneBuild build;
		if (!importResult.IsValid())
		{
			build.errorMessage = "Scene import result is not valid";
			return build;
		}

		std::filesystem::path resolvedSourceScenePath;
		if (!ResolveSourceScenePath(sourceScenePath, resolvedSourceScenePath, build.errorMessage))
		{
			return build;
		}

		if (!BuildSceneAssetId(resolvedSourceScenePath, build.sceneAssetId, build.errorMessage))
		{
			return build;
		}

		build.sceneManifestPath = Paths::CookedSceneManifest(build.sceneAssetId);
		BuildMeshAssets(importResult, build.sceneAssetId, build);
		if (!BuildMaterialAssets(importResult, build.sceneAssetId, build, build.errorMessage))
		{
			return build;
		}

		if (!BuildManifest(importResult, build, build.errorMessage))
		{
			return build;
		}

		if (!WriteBuildOutputs(build, build.errorMessage))
		{
			return build;
		}

		return build;
	}

	bool CookedSceneCooker::CollectTextureCookRequests(
	    const SceneImportResult& importResult,
	    std::vector<TextureCookRequest>& outRequests,
	    std::string& outErrorMessage) const
	{
		outRequests.clear();
		std::unordered_set<TextureAssetId> referencedTextureAssetIds;
		referencedTextureAssetIds.reserve(importResult.materials.size() * 8);

		auto appendTextureRequest = [&](const SceneImportResult::MaterialTextureSource& textureSource) -> bool
		{
			if (textureSource.sourcePath.empty())
			{
				return true;
			}

			TextureCookRequest request;
			if (!TextureCookRequestBuilder::Build(
			        textureSource.sourcePath,
			        textureSource.textureGroup,
			        request,
			        outErrorMessage,
			        textureSource.channelMask))
			{
				return false;
			}

			if (referencedTextureAssetIds.insert(request.assetId).second)
			{
				outRequests.push_back(std::move(request));
			}

			return true;
		};

		auto appendFallbackTextureRequest = [&](const std::optional<std::filesystem::path>& texturePath,
		                                        TextureGroup textureGroup,
		                                        TextureChannelMask channelMask = TextureChannelMask::Rgba) -> bool
		{
			if (!texturePath)
			{
				return true;
			}

			return appendTextureRequest({textureGroup, *texturePath, channelMask});
		};

		for (std::size_t materialIndex = 0; materialIndex < importResult.materials.size(); ++materialIndex)
		{
			if (materialIndex < importResult.materialTextureSources.size() && !importResult.materialTextureSources[materialIndex].empty())
			{
				for (const SceneImportResult::MaterialTextureSource& textureSource : importResult.materialTextureSources[materialIndex])
				{
					if (!appendTextureRequest(textureSource))
					{
						return false;
					}
				}

				continue;
			}

			const MaterialDesc& materialDesc = importResult.materials[materialIndex];
			if (!appendFallbackTextureRequest(materialDesc.albedoTexture, TextureGroup::Diffuse) ||
			    !appendFallbackTextureRequest(materialDesc.normalTexture, TextureGroup::NormalMap) ||
			    !appendFallbackTextureRequest(materialDesc.roughnessTexture, TextureGroup::Roughness, TextureChannelMask::Red) ||
			    !appendFallbackTextureRequest(materialDesc.metallicTexture, TextureGroup::Metallic, TextureChannelMask::Red) ||
			    !appendFallbackTextureRequest(materialDesc.occlusionTexture, TextureGroup::AmbientOcclusion, TextureChannelMask::Red) ||
			    !appendFallbackTextureRequest(materialDesc.emissiveTexture, TextureGroup::Emissive) ||
			    !appendFallbackTextureRequest(materialDesc.subsurfaceColorTexture, TextureGroup::SubsurfaceColor) ||
			    !appendFallbackTextureRequest(
			        materialDesc.subsurfaceStrengthTexture,
			        TextureGroup::SubsurfaceStrength,
			        TextureChannelMask::Red))
			{
				return false;
			}
		}

		outErrorMessage.clear();
		return true;
	}

	bool CookedSceneCooker::ResolveSourceScenePath(
	    const std::filesystem::path& sourceScenePath,
	    std::filesystem::path& outResolvedPath,
	    std::string& outErrorMessage)
	{
		if (const auto resolvedPath = Filesystem::ResolveAssetPathNormalized(sourceScenePath, AssetType::Mesh))
		{
			outResolvedPath = *resolvedPath;
			outErrorMessage.clear();
			return true;
		}

		outErrorMessage = "Unable to resolve source scene path '" + sourceScenePath.string() + "'";
		return false;
	}

	bool CookedSceneCooker::BuildSceneAssetId(
	    const std::filesystem::path& resolvedSourceScenePath,
	    std::string& outSceneAssetId,
	    std::string& outErrorMessage)
	{
		const std::filesystem::path projectMeshRoot = Paths::TypedAssetRoot(AssetType::Mesh, PathRoot::Project);
		const std::filesystem::path engineMeshRoot = Paths::TypedAssetRoot(AssetType::Mesh, PathRoot::Engine);

		std::optional<std::filesystem::path> relativePath = Paths::TryMakeRelativeUnderRoot(resolvedSourceScenePath, projectMeshRoot);
		if (!relativePath)
		{
			relativePath = Paths::TryMakeRelativeUnderRoot(resolvedSourceScenePath, engineMeshRoot);
		}

		if (!relativePath)
		{
			outErrorMessage = "Source scene path must be under a Sparkle mesh asset root to derive a stable scene asset id: '" +
			                  resolvedSourceScenePath.string() + "'";
			return false;
		}

		outSceneAssetId = relativePath->generic_string();
		std::filesystem::path sceneAssetPath(outSceneAssetId);
		sceneAssetPath.replace_extension();
		outSceneAssetId = sceneAssetPath.generic_string();
		outErrorMessage.clear();
		return true;
	}

	Assets::CookedAssetId CookedSceneCooker::BuildMeshAssetId(std::string_view sceneAssetId, std::size_t meshIndex) noexcept
	{
		return Hash::Fnv1a64(std::string(sceneAssetId) + "#mesh#" + std::to_string(meshIndex));
	}

	Assets::CookedAssetId CookedSceneCooker::BuildMaterialAssetId(std::string_view sceneAssetId, std::size_t materialIndex) noexcept
	{
		return Hash::Fnv1a64(std::string(sceneAssetId) + "#material#" + std::to_string(materialIndex));
	}

	Assets::CookedAlphaMode CookedSceneCooker::TranslateAlphaMode(AlphaMode alphaMode) noexcept
	{
		switch (alphaMode)
		{
			case AlphaMode::Opaque:
				return Assets::CookedAlphaMode::Opaque;
			case AlphaMode::Mask:
				return Assets::CookedAlphaMode::Mask;
			case AlphaMode::Blend:
				return Assets::CookedAlphaMode::Blend;
		}

		return Assets::CookedAlphaMode::Opaque;
	}

	void CookedSceneCooker::BuildMeshAssets(
	    const SceneImportResult& importResult,
	    std::string_view sceneAssetId,
	    CookedSceneBuild& outBuild)
	{
		outBuild.meshAssets.reserve(importResult.meshes.size());
		outBuild.meshAssetReferences.reserve(importResult.meshes.size());

		for (std::size_t meshIndex = 0; meshIndex < importResult.meshes.size(); ++meshIndex)
		{
			const MeshData& meshData = importResult.meshes[meshIndex];
			CookedMeshAssetBuild meshAsset;
			meshAsset.assetId = BuildMeshAssetId(sceneAssetId, meshIndex);
			meshAsset.vertices.reserve(meshData.vertices.size());
			for (const VertexData& vertex : meshData.vertices)
			{
				meshAsset.vertices.push_back(
				    Assets::CookedMeshVertex{
				        .position = vertex.position,
				        .uv = vertex.uv,
				        .color = vertex.color,
				        .normal = vertex.normal,
				        .tangent = vertex.tangent});
			}
			meshAsset.indices = meshData.indices;

			outBuild.meshAssetReferences.push_back({meshAsset.assetId});
			outBuild.meshAssets.push_back(std::move(meshAsset));
		}
	}

	bool CookedSceneCooker::BuildMaterialAssets(
	    const SceneImportResult& importResult,
	    std::string_view sceneAssetId,
	    CookedSceneBuild& outBuild,
	    std::string& outErrorMessage)
	{
		outBuild.materialAssets.reserve(importResult.materials.size());
		outBuild.materialAssetReferences.reserve(importResult.materials.size());

		auto appendTextureReference = [&](const SceneImportResult::MaterialTextureSource& textureSource,
		                                  CookedMaterialAssetBuild& materialAsset) -> bool
		{
			if (textureSource.sourcePath.empty())
			{
				return true;
			}

			TextureCookRequest request;
			if (!TextureCookRequestBuilder::Build(
			        textureSource.sourcePath,
			        textureSource.textureGroup,
			        request,
			        outErrorMessage,
			        textureSource.channelMask))
			{
				return false;
			}

			materialAsset.textureReferences.push_back({static_cast<Assets::CookedAssetId>(request.assetId), textureSource.textureGroup});
			return true;
		};

		auto appendFallbackTextureReference = [&](const std::optional<std::filesystem::path>& texturePath,
		                                          TextureGroup textureGroup,
		                                          CookedMaterialAssetBuild& materialAsset,
		                                          TextureChannelMask channelMask = TextureChannelMask::Rgba) -> bool
		{
			if (!texturePath)
			{
				return true;
			}

			return appendTextureReference({textureGroup, *texturePath, channelMask}, materialAsset);
		};

		for (std::size_t materialIndex = 0; materialIndex < importResult.materials.size(); ++materialIndex)
		{
			const MaterialDesc& materialDesc = importResult.materials[materialIndex];
			CookedMaterialAssetBuild materialAsset;
			materialAsset.assetId = BuildMaterialAssetId(sceneAssetId, materialIndex);
			materialAsset.name = materialDesc.name;
			materialAsset.header.nameByteCount = static_cast<std::uint32_t>(materialAsset.name.size());
			materialAsset.header.textureReferenceCount = 0;
			materialAsset.header.alphaMode = TranslateAlphaMode(materialDesc.alphaMode);
			materialAsset.header.baseColor = materialDesc.baseColor;
			materialAsset.header.metallic = materialDesc.metallic;
			materialAsset.header.roughness = materialDesc.roughness;
			materialAsset.header.f0 = materialDesc.f0;
			materialAsset.header.subsurfaceColor = materialDesc.subsurfaceColor;
			materialAsset.header.subsurfaceStrength = materialDesc.subsurfaceStrength;
			materialAsset.header.alphaCutoff = materialDesc.alphaCutoff;
			materialAsset.header.emissiveColor = materialDesc.emissiveColor;

			if (materialIndex < importResult.materialTextureSources.size() && !importResult.materialTextureSources[materialIndex].empty())
			{
				for (const SceneImportResult::MaterialTextureSource& textureSource : importResult.materialTextureSources[materialIndex])
				{
					if (!appendTextureReference(textureSource, materialAsset))
					{
						return false;
					}
				}
			}
			else if (
			    !appendFallbackTextureReference(materialDesc.albedoTexture, TextureGroup::Diffuse, materialAsset) ||
			    !appendFallbackTextureReference(materialDesc.normalTexture, TextureGroup::NormalMap, materialAsset) ||
			    !appendFallbackTextureReference(
			        materialDesc.roughnessTexture,
			        TextureGroup::Roughness,
			        materialAsset,
			        TextureChannelMask::Red) ||
			    !appendFallbackTextureReference(
			        materialDesc.metallicTexture,
			        TextureGroup::Metallic,
			        materialAsset,
			        TextureChannelMask::Red) ||
			    !appendFallbackTextureReference(
			        materialDesc.occlusionTexture,
			        TextureGroup::AmbientOcclusion,
			        materialAsset,
			        TextureChannelMask::Red) ||
			    !appendFallbackTextureReference(materialDesc.emissiveTexture, TextureGroup::Emissive, materialAsset) ||
			    !appendFallbackTextureReference(materialDesc.subsurfaceColorTexture, TextureGroup::SubsurfaceColor, materialAsset) ||
			    !appendFallbackTextureReference(
			        materialDesc.subsurfaceStrengthTexture,
			        TextureGroup::SubsurfaceStrength,
			        materialAsset,
			        TextureChannelMask::Red))
			{
				return false;
			}

			materialAsset.header.textureReferenceCount = static_cast<std::uint32_t>(materialAsset.textureReferences.size());

			outBuild.materialAssetReferences.push_back({materialAsset.assetId});
			outBuild.materialAssets.push_back(std::move(materialAsset));
		}

		outErrorMessage.clear();
		return true;
	}

	bool CookedSceneCooker::BuildManifest(const SceneImportResult& importResult, CookedSceneBuild& outBuild, std::string& outErrorMessage)
	{
		outBuild.instances.reserve(importResult.meshes.size());

		for (std::size_t meshIndex = 0; meshIndex < importResult.meshes.size(); ++meshIndex)
		{
			const Transform instanceTransform =
			    meshIndex < importResult.transforms.size() ? importResult.transforms[meshIndex] : Transform();

			std::uint32_t materialAssetIndex = Assets::kInvalidCookedMaterialAssetIndex;
			if (meshIndex < importResult.materialHandles.size() && importResult.materialHandles[meshIndex].IsValid())
			{
				materialAssetIndex = importResult.materialHandles[meshIndex].GetIndex();
				if (materialAssetIndex >= outBuild.materialAssets.size())
				{
					outErrorMessage = "Imported mesh instance references a material index outside the imported material set";
					return false;
				}
			}

			outBuild.instances.push_back(
			    Assets::CookedSceneInstanceRecord{
			        .meshAssetIndex = static_cast<std::uint32_t>(meshIndex),
			        .materialAssetIndex = materialAssetIndex,
			        .worldTransform = instanceTransform.GetWorldMatrix4x4()});
		}

		outBuild.manifestHeader.meshAssetReferenceCount = static_cast<std::uint32_t>(outBuild.meshAssetReferences.size());
		outBuild.manifestHeader.materialAssetReferenceCount = static_cast<std::uint32_t>(outBuild.materialAssetReferences.size());
		outBuild.manifestHeader.instanceCount = static_cast<std::uint32_t>(outBuild.instances.size());
		outErrorMessage.clear();
		return true;
	}

	bool CookedSceneCooker::WriteBuildOutputs(const CookedSceneBuild& build, std::string& outErrorMessage)
	{
		for (const CookedMeshAssetBuild& meshAsset : build.meshAssets)
		{
			const std::filesystem::path outputPath = Paths::CookedMeshAsset(meshAsset.assetId);
			const Assets::CookedMeshAssetHeader header{
			    .fileHeader = {Assets::kCookedMeshAssetMagic, Assets::kCookedMeshAssetVersion},
			    .vertexCount = static_cast<std::uint32_t>(meshAsset.vertices.size()),
			    .indexCount = static_cast<std::uint32_t>(meshAsset.indices.size()),
			    .vertexStride = sizeof(Assets::CookedMeshVertex),
			    .indexStride = sizeof(std::uint32_t)};
			const Cook::CookArtifactKey artifactKey = BuildMeshCookArtifactKey(meshAsset, header, outputPath);
			bool isCurrent = false;
			isCurrent = Cook::CookArtifactCache::IsCurrent(artifactKey, outErrorMessage);
			if (!isCurrent && !outErrorMessage.empty())
			{
				return false;
			}
			if (isCurrent)
			{
				continue;
			}

			std::ofstream output;
			if (!Files::TryOpenBinaryOutput(outputPath, output, outErrorMessage))
			{
				return false;
			}

			if (!Files::BinaryStreamWriter::WriteValue(output, header, outErrorMessage) ||
			    !Files::BinaryStreamWriter::WriteArray(output, meshAsset.vertices, outErrorMessage) ||
			    !Files::BinaryStreamWriter::WriteArray(output, meshAsset.indices, outErrorMessage))
			{
				return false;
			}

			if (!Files::TryCloseOutput(output, outputPath, outErrorMessage))
			{
				return false;
			}

			if (!Cook::CookArtifactCache::Publish(artifactKey, outErrorMessage))
			{
				return false;
			}
		}

		for (const CookedMaterialAssetBuild& materialAsset : build.materialAssets)
		{
			const std::filesystem::path outputPath = Paths::CookedMaterialAsset(materialAsset.assetId);
			const Cook::CookArtifactKey artifactKey = BuildMaterialCookArtifactKey(materialAsset, outputPath);
			bool isCurrent = false;
			isCurrent = Cook::CookArtifactCache::IsCurrent(artifactKey, outErrorMessage);
			if (!isCurrent && !outErrorMessage.empty())
			{
				return false;
			}
			if (isCurrent)
			{
				continue;
			}

			std::ofstream output;
			if (!Files::TryOpenBinaryOutput(outputPath, output, outErrorMessage))
			{
				return false;
			}

			if (!Files::BinaryStreamWriter::WriteValue(output, materialAsset.header, outErrorMessage))
			{
				return false;
			}

			if (!materialAsset.name.empty())
			{
				output.write(materialAsset.name.data(), static_cast<std::streamsize>(materialAsset.name.size()));
				if (!output.good())
				{
					outErrorMessage = "Failed to write cooked material asset name payload";
					return false;
				}
			}

			if (!Files::BinaryStreamWriter::WriteArray(output, materialAsset.textureReferences, outErrorMessage))
			{
				return false;
			}

			if (!Files::TryCloseOutput(output, outputPath, outErrorMessage))
			{
				return false;
			}

			if (!Cook::CookArtifactCache::Publish(artifactKey, outErrorMessage))
			{
				return false;
			}
		}

		const Cook::CookArtifactKey manifestArtifactKey = BuildSceneManifestCookArtifactKey(build);
		bool manifestIsCurrent = false;
		manifestIsCurrent = Cook::CookArtifactCache::IsCurrent(manifestArtifactKey, outErrorMessage);
		if (!manifestIsCurrent && !outErrorMessage.empty())
		{
			return false;
		}
		if (!manifestIsCurrent)
		{
			std::ofstream manifestOutput;
			if (!Files::TryOpenBinaryOutput(build.sceneManifestPath, manifestOutput, outErrorMessage))
			{
				return false;
			}

			if (!Files::BinaryStreamWriter::WriteValue(manifestOutput, build.manifestHeader, outErrorMessage) ||
			    !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.meshAssetReferences, outErrorMessage) ||
			    !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.materialAssetReferences, outErrorMessage) ||
			    !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.instances, outErrorMessage))
			{
				return false;
			}

			if (!Files::TryCloseOutput(manifestOutput, build.sceneManifestPath, outErrorMessage))
			{
				return false;
			}

			if (!Cook::CookArtifactCache::Publish(manifestArtifactKey, outErrorMessage))
			{
				return false;
			}
		}

		if (!UpdateSceneAssetRegistry(build, outErrorMessage))
		{
			return false;
		}

		outErrorMessage.clear();
		return true;
	}

	bool CookedSceneCooker::UpdateSceneAssetRegistry(const CookedSceneBuild& build, std::string& outErrorMessage)
	{
		const std::filesystem::path manifestRoot = Paths::CookedSceneManifestRoot();
		const std::optional<std::filesystem::path> manifestRelativePath =
		    Paths::TryMakeRelativeUnderRoot(build.sceneManifestPath, manifestRoot);
		if (!manifestRelativePath)
		{
			outErrorMessage = "Failed to derive a relative cooked scene manifest path for scene asset id '" + build.sceneAssetId + "'";
			return false;
		}

		Assets::SceneAssetRegistry sceneAssetRegistry;
		if (!sceneAssetRegistry.Load(outErrorMessage))
		{
			return false;
		}

		sceneAssetRegistry.Upsert(build.sceneAssetId, *manifestRelativePath);
		if (!sceneAssetRegistry.Save(outErrorMessage))
		{
			return false;
		}

		outErrorMessage.clear();
		return true;
	}