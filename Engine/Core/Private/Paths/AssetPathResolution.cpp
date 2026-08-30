#include "PCH.h"

#include "Core/Public/FileSystemUtils.h"

#include "AssetPathState.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Core/Public/Strings/StringUtils.h"

#include <system_error>
#include <utility>

class AssetPathSearch final
{
public:
	static std::optional<std::filesystem::path> TryResolveIn(
	    const std::filesystem::path& searchDirectory,
	    const std::filesystem::path& relativePath,
	    AssetType type)
	{
		if (searchDirectory.empty())
		{
			return std::nullopt;
		}

		std::error_code errorCode;
		const std::string_view assetSubdirectory = GetAssetSubdirectory(type);
		if (!assetSubdirectory.empty())
		{
			const std::filesystem::path typedCandidate = searchDirectory / assetSubdirectory / relativePath;
			if (std::filesystem::exists(typedCandidate, errorCode))
			{
				return std::filesystem::weakly_canonical(typedCandidate);
			}
		}

		const std::filesystem::path directCandidate = searchDirectory / relativePath;
		if (std::filesystem::exists(directCandidate, errorCode))
		{
			return std::filesystem::weakly_canonical(directCandidate);
		}
		return std::nullopt;
	}
};

namespace Filesystem
{
	const std::filesystem::path& GetTypedPath(AssetType type, PathRoot root) noexcept
	{
		Private::AssetPathState& state = Private::GetAssetPathState();
		const std::size_t typeIndex = static_cast<std::size_t>(type);
		if (type == AssetType::Count || typeIndex >= Private::AssetPathState::AssetTypeCount)
		{
			return state.emptyPath;
		}

		switch (root)
		{
			case PathRoot::Project:
				return state.projectTypedPaths[typeIndex];
			case PathRoot::Engine:
				return state.engineTypedPaths[typeIndex];
			case PathRoot::Any:
			default:
				return state.projectTypedPaths[typeIndex].empty() ? state.engineTypedPaths[typeIndex] : state.projectTypedPaths[typeIndex];
		}
	}

	const std::filesystem::path& GetShaderPath(PathRoot root) noexcept
	{
		return GetTypedPath(AssetType::Shader, root);
	}

	std::optional<std::filesystem::path> ResolveAssetPathNormalized(const std::filesystem::path& inputPath, AssetType type)
	{
		if (const auto resolvedPath = ResolveAssetPath(inputPath, type))
		{
			return Paths::Normalize(*resolvedPath);
		}
		return std::nullopt;
	}

	void AppendNormalizedAssetPaths(
	    std::span<const std::filesystem::path> inputPaths,
	    AssetType type,
	    std::vector<std::filesystem::path>& destination)
	{
		for (const std::filesystem::path& inputPath : inputPaths)
		{
			std::filesystem::path normalizedPath = ResolveAssetPathNormalized(inputPath, type).value_or(Paths::Normalize(inputPath));
			if (!normalizedPath.empty())
			{
				destination.push_back(std::move(normalizedPath));
			}
		}
	}

	std::optional<std::filesystem::path> ResolveAssetPath(const std::filesystem::path& inputPath, AssetType type)
	{
		if (inputPath.empty())
		{
			return std::nullopt;
		}

		if (inputPath.is_absolute())
		{
			std::error_code errorCode;
			return std::filesystem::exists(inputPath, errorCode) ? std::make_optional(inputPath) : std::nullopt;
		}

		Private::AssetPathState& state = Private::GetAssetPathState();
		if (type == AssetType::Texture && Strings::EqualsIgnoreCase(inputPath.extension().string(), ".stex"))
		{
			if (const auto cookedPath = AssetPathSearch::TryResolveIn(state.cookedAssetRootPath, inputPath, type))
			{
				return cookedPath;
			}
		}
		if (const auto projectPath = AssetPathSearch::TryResolveIn(state.projectAssetsPath, inputPath, type))
		{
			return projectPath;
		}
		return AssetPathSearch::TryResolveIn(state.engineAssetsPath, inputPath, type);
	}

	std::filesystem::path ResolveAssetPathValidated(const std::filesystem::path& inputPath, AssetType type)
	{
		if (const auto resolvedPath = ResolveAssetPath(inputPath, type))
		{
			return *resolvedPath;
		}

		Diagnostics::Fatal(
		    Logging::GetOrCreateLogger("Core.FileSystem"),
		    __FILE__,
		    __LINE__,
		    std::string(GetAssetTypeName(type)) + " asset not found: " + inputPath.string());
		return {};
	}
}
