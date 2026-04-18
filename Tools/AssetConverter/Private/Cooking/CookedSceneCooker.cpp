#include "PCH.h"

#include "Cooking/CookedSceneCooker.h"

#include "Cooking/TextureCookRequestBuilder.h"

#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Assets/SceneAssetRegistry.h"

#include <format>
#include <fstream>
#include <limits>
#include <unordered_set>
#include <system_error>

namespace Engine::AssetAuthoring
{
	namespace
	{
		template <typename T> bool WriteValue(std::ofstream& output, const T& value, std::string& outErrorMessage)
		{
			output.write(reinterpret_cast<const char*>(&value), sizeof(T));
			if (output.good())
			{
				return true;
			}

			outErrorMessage = "Failed to write cooked binary payload";
			return false;
		}

		template <typename T>
		bool WriteArray(std::ofstream& output, const std::vector<T>& values, std::string& outErrorMessage)
		{
			if (values.empty())
			{
				return true;
			}

			output.write(reinterpret_cast<const char*>(values.data()), static_cast<std::streamsize>(sizeof(T) * values.size()));
			if (output.good())
			{
				return true;
			}

			outErrorMessage = "Failed to write cooked binary array payload";
			return false;
		}

		bool OpenBinaryOutput(const std::filesystem::path& path, std::ofstream& output, std::string& outErrorMessage)
		{
			std::error_code errorCode;
			std::filesystem::create_directories(path.parent_path(), errorCode);
			if (errorCode)
			{
				outErrorMessage = "Failed to create output directory '" + path.parent_path().string() + "'";
				return false;
			}

			output.open(path, std::ios::binary | std::ios::trunc);
			if (output.is_open())
			{
				return true;
			}

			outErrorMessage = "Failed to open cooked output '" + path.string() + "'";
			return false;
		}
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

		build.sceneManifestPath = BuildSceneManifestPath(build.sceneAssetId);
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
		referencedTextureAssetIds.reserve(importResult.materials.size() * 5);

		auto appendTextureRequest =
		    [&](const std::optional<std::filesystem::path>& texturePath, Engine::Assets::CookedTextureSemantic semantic) -> bool
		{
			if (!texturePath)
			{
				return true;
			}

			TextureCookRequest request;
			if (!TextureCookRequestBuilder::Build(*texturePath, semantic, request, outErrorMessage))
			{
				return false;
			}

			if (referencedTextureAssetIds.insert(request.assetId).second)
			{
				outRequests.push_back(std::move(request));
			}

			return true;
		};

		for (const MaterialDesc& materialDesc : importResult.materials)
		{
			if (!appendTextureRequest(materialDesc.albedoTexture, Engine::Assets::CookedTextureSemantic::Albedo) ||
			    !appendTextureRequest(materialDesc.normalTexture, Engine::Assets::CookedTextureSemantic::Normal) ||
			    !appendTextureRequest(
			        materialDesc.metallicRoughnessTexture,
			        Engine::Assets::CookedTextureSemantic::MetallicRoughness) ||
			    !appendTextureRequest(materialDesc.occlusionTexture, Engine::Assets::CookedTextureSemantic::Occlusion) ||
			    !appendTextureRequest(materialDesc.emissiveTexture, Engine::Assets::CookedTextureSemantic::Emissive))
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

		const std::filesystem::path normalizedAbsolutePath = Engine::Paths::Normalize(sourceScenePath);
		if (!normalizedAbsolutePath.empty() && normalizedAbsolutePath.is_absolute())
		{
			std::error_code errorCode;
			if (std::filesystem::exists(normalizedAbsolutePath, errorCode) && !errorCode)
			{
				outResolvedPath = normalizedAbsolutePath;
				outErrorMessage.clear();
				return true;
			}
		}

		outErrorMessage = "Unable to resolve source scene path '" + sourceScenePath.string() + "'";
		return false;
	}

	bool CookedSceneCooker::BuildSceneAssetId(
	    const std::filesystem::path& resolvedSourceScenePath,
	    std::string& outSceneAssetId,
	    std::string& outErrorMessage)
	{
		std::error_code errorCode;
		const std::filesystem::path projectMeshRoot = Filesystem::GetTypedPath(AssetType::Mesh, PathRoot::Project);
		const std::filesystem::path engineMeshRoot = Filesystem::GetTypedPath(AssetType::Mesh, PathRoot::Engine);

		std::filesystem::path relativePath = std::filesystem::relative(resolvedSourceScenePath, projectMeshRoot, errorCode);
		if (const std::string relativePathString = relativePath.generic_string();
		    errorCode || relativePathString.empty() || relativePathString.starts_with(".."))
		{
			errorCode.clear();
			relativePath = std::filesystem::relative(resolvedSourceScenePath, engineMeshRoot, errorCode);
		}

		if (const std::string relativePathString = relativePath.generic_string();
		    errorCode || relativePathString.empty() || relativePathString.starts_with(".."))
		{
			outErrorMessage =
			    "Source scene path must be under a Sparkle mesh asset root to derive a stable scene asset id: '" +
			    resolvedSourceScenePath.string() + "'";
			return false;
		}

		outSceneAssetId = relativePath.generic_string();
		std::filesystem::path sceneAssetPath(outSceneAssetId);
		sceneAssetPath.replace_extension();
		outSceneAssetId = sceneAssetPath.generic_string();
		outErrorMessage.clear();
		return true;
	}

	Engine::Assets::CookedAssetId CookedSceneCooker::BuildMeshAssetId(std::string_view sceneAssetId, std::size_t meshIndex) noexcept
	{
		return Hash::Fnv1a64(std::string(sceneAssetId) + "#mesh#" + std::to_string(meshIndex));
	}

	Engine::Assets::CookedAssetId CookedSceneCooker::BuildMaterialAssetId(
	    std::string_view sceneAssetId,
	    std::size_t materialIndex) noexcept
	{
		return Hash::Fnv1a64(std::string(sceneAssetId) + "#material#" + std::to_string(materialIndex));
	}

	std::filesystem::path CookedSceneCooker::BuildSceneManifestPath(std::string_view sceneAssetId)
	{
		std::filesystem::path relativeScenePath(sceneAssetId);
		relativeScenePath.replace_extension(".sscn");
		return Filesystem::GetProjectAssetsPath() / "Cooked" / "SceneManifests" / relativeScenePath;
	}

	std::filesystem::path CookedSceneCooker::BuildMeshAssetPath(Engine::Assets::CookedAssetId meshAssetId)
	{
		return Filesystem::GetProjectAssetsPath() / "Cooked" / "Meshes" /
		       (std::format("{:016X}", meshAssetId) + ".smsh");
	}

	std::filesystem::path CookedSceneCooker::BuildMaterialAssetPath(Engine::Assets::CookedAssetId materialAssetId)
	{
		return Filesystem::GetProjectAssetsPath() / "Cooked" / "Materials" /
		       (std::format("{:016X}", materialAssetId) + ".smat");
	}

	Engine::Assets::CookedAlphaMode CookedSceneCooker::TranslateAlphaMode(AlphaMode alphaMode) noexcept
	{
		switch (alphaMode)
		{
			case AlphaMode::Opaque:
				return Engine::Assets::CookedAlphaMode::Opaque;
			case AlphaMode::Mask:
				return Engine::Assets::CookedAlphaMode::Mask;
			case AlphaMode::Blend:
				return Engine::Assets::CookedAlphaMode::Blend;
		}

		return Engine::Assets::CookedAlphaMode::Opaque;
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
				    Engine::Assets::CookedMeshVertex{
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

		auto appendTextureReference =
		    [&](const std::optional<std::filesystem::path>& texturePath,
		        Engine::Assets::CookedTextureSemantic semantic,
		        CookedMaterialAssetBuild& materialAsset) -> bool
		{
			if (!texturePath)
			{
				return true;
			}

			TextureCookRequest request;
			if (!TextureCookRequestBuilder::Build(*texturePath, semantic, request, outErrorMessage))
			{
				return false;
			}

			materialAsset.textureReferences.push_back(
			    {static_cast<Engine::Assets::CookedAssetId>(request.assetId), semantic});
			return true;
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
			materialAsset.header.alphaCutoff = materialDesc.alphaCutoff;
			materialAsset.header.emissiveColor = materialDesc.emissiveColor;

			if (!appendTextureReference(materialDesc.albedoTexture, Engine::Assets::CookedTextureSemantic::Albedo, materialAsset) ||
			    !appendTextureReference(materialDesc.normalTexture, Engine::Assets::CookedTextureSemantic::Normal, materialAsset) ||
			    !appendTextureReference(
			        materialDesc.metallicRoughnessTexture,
			        Engine::Assets::CookedTextureSemantic::MetallicRoughness,
			        materialAsset) ||
			    !appendTextureReference(materialDesc.occlusionTexture, Engine::Assets::CookedTextureSemantic::Occlusion, materialAsset) ||
			    !appendTextureReference(materialDesc.emissiveTexture, Engine::Assets::CookedTextureSemantic::Emissive, materialAsset))
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

	bool CookedSceneCooker::BuildManifest(
	    const SceneImportResult& importResult,
	    CookedSceneBuild& outBuild,
	    std::string& outErrorMessage)
	{
		outBuild.instances.reserve(importResult.meshes.size());

		for (std::size_t meshIndex = 0; meshIndex < importResult.meshes.size(); ++meshIndex)
		{
			const Transform instanceTransform =
			    meshIndex < importResult.transforms.size() ? importResult.transforms[meshIndex] : Transform();

			std::uint32_t materialAssetIndex = Engine::Assets::kInvalidCookedMaterialAssetIndex;
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
			    Engine::Assets::CookedSceneInstanceRecord{
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
			std::ofstream output;
			const std::filesystem::path outputPath = BuildMeshAssetPath(meshAsset.assetId);
			if (!OpenBinaryOutput(outputPath, output, outErrorMessage))
			{
				return false;
			}

			const Engine::Assets::CookedMeshAssetHeader header{
			    .fileHeader = {Engine::Assets::kCookedMeshAssetMagic, Engine::Assets::kCookedMeshAssetVersion},
			    .vertexCount = static_cast<std::uint32_t>(meshAsset.vertices.size()),
			    .indexCount = static_cast<std::uint32_t>(meshAsset.indices.size()),
			    .vertexStride = sizeof(Engine::Assets::CookedMeshVertex),
			    .indexStride = sizeof(std::uint32_t)};

			if (!WriteValue(output, header, outErrorMessage) || !WriteArray(output, meshAsset.vertices, outErrorMessage) ||
			    !WriteArray(output, meshAsset.indices, outErrorMessage))
			{
				return false;
			}
		}

		for (const CookedMaterialAssetBuild& materialAsset : build.materialAssets)
		{
			std::ofstream output;
			const std::filesystem::path outputPath = BuildMaterialAssetPath(materialAsset.assetId);
			if (!OpenBinaryOutput(outputPath, output, outErrorMessage))
			{
				return false;
			}

			if (!WriteValue(output, materialAsset.header, outErrorMessage))
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

			if (!WriteArray(output, materialAsset.textureReferences, outErrorMessage))
			{
				return false;
			}
		}

		std::ofstream manifestOutput;
		if (!OpenBinaryOutput(build.sceneManifestPath, manifestOutput, outErrorMessage))
		{
			return false;
		}

		if (!WriteValue(manifestOutput, build.manifestHeader, outErrorMessage) ||
		    !WriteArray(manifestOutput, build.meshAssetReferences, outErrorMessage) ||
		    !WriteArray(manifestOutput, build.materialAssetReferences, outErrorMessage) ||
		    !WriteArray(manifestOutput, build.instances, outErrorMessage))
		{
			return false;
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
		std::error_code errorCode;
		const std::filesystem::path manifestRoot = Filesystem::GetProjectAssetsPath() / "Cooked" / "SceneManifests";
		const std::filesystem::path manifestRelativePath = std::filesystem::relative(build.sceneManifestPath, manifestRoot, errorCode);
		const std::string manifestRelativePathString = manifestRelativePath.generic_string();
		if (errorCode || manifestRelativePathString.empty() || manifestRelativePathString.starts_with(".."))
		{
			outErrorMessage = "Failed to derive a relative cooked scene manifest path for scene asset id '" + build.sceneAssetId + "'";
			return false;
		}

		Engine::Assets::SceneAssetRegistry sceneAssetRegistry;
		if (!sceneAssetRegistry.Load(outErrorMessage))
		{
			return false;
		}

		sceneAssetRegistry.Upsert(build.sceneAssetId, manifestRelativePath);
		if (!sceneAssetRegistry.Save(outErrorMessage))
		{
			return false;
		}

		outErrorMessage.clear();
		return true;
	}
}