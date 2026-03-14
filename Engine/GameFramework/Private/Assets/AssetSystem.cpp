#include "PCH.h"

#include "Assets/AssetSystem.h"

#include "FileSystemUtils.h"

AssetSystem::AssetSystem()
{
	m_workingDirectory = std::filesystem::current_path();
	m_executableDirectory = Engine::FileSystem::GetExecutableDirectory();

	DiscoverPaths();

	m_projectPath = Engine::FileSystem::NormalizePath(m_projectPath);
	m_projectAssetsPath = Engine::FileSystem::NormalizePath(m_projectAssetsPath);
	m_enginePath = Engine::FileSystem::NormalizePath(m_enginePath);
	m_engineAssetsPath = Engine::FileSystem::NormalizePath(m_engineAssetsPath);

	InitializeTypedPaths();
	InitializeOutputPaths();
	ValidatePaths();
}

void AssetSystem::DiscoverPaths()
{
	if (auto engineRoot = Engine::FileSystem::DiscoverEngineRoot())
	{
		m_enginePath = *engineRoot;
		m_engineAssetsPath = m_enginePath / "Assets";
	}

	if (auto projectRoot = Engine::FileSystem::DiscoverProjectRoot())
	{
		m_projectPath = *projectRoot;
		m_projectAssetsPath = m_projectPath / "Assets";
	}
}

void AssetSystem::InitializeTypedPaths()
{
	auto buildTypedPaths = [](const std::filesystem::path& root, std::array<std::filesystem::path, kAssetTypeCount>& paths)
	{
		if (root.empty())
		{
			return;
		}

		for (size_t i = 0; i < kAssetTypeCount; ++i)
		{
			const auto type = static_cast<AssetType>(i);
			const auto subdir = GetAssetSubdirectory(type);
			paths[i] = subdir.empty() ? root : root / subdir;
		}
	};

	buildTypedPaths(m_projectAssetsPath, m_projectTypedPaths);
	buildTypedPaths(m_engineAssetsPath, m_engineTypedPaths);
}

void AssetSystem::InitializeOutputPaths()
{
	const auto& outputRoot = HasProjectAssets() ? m_projectAssetsPath : m_engineAssetsPath;

	if (!outputRoot.empty())
	{
		m_shaderSymbolsOutputPath = outputRoot / GetAssetSubdirectory(AssetType::ShaderSymbols);

		std::error_code ec;
		std::filesystem::create_directories(m_shaderSymbolsOutputPath, ec);
	}
}

void AssetSystem::ValidatePaths()
{
	std::error_code ec;

	auto logPath = [&](const char* label, const std::filesystem::path& path, bool required)
	{
		constexpr int kLabelWidth = 24;
		const std::string paddedLabel = std::string(label) + std::string(kLabelWidth - strlen(label), ' ');

		if (path.empty())
		{
			if (required)
			{
				LOG_FATAL("[MISSING]  " + paddedLabel + ": (not configured)");
			}
			else
			{
				LOG_INFO("[--]       " + paddedLabel + ": (not configured)");
			}
			return;
		}

		const bool exists = std::filesystem::exists(path, ec);
		if (exists)
		{
			LOG_INFO("[OK]       " + paddedLabel + ": " + path.string());
			return;
		}

		if (required)
		{
			LOG_FATAL("[MISSING]  " + paddedLabel + ": " + path.string());
		}
		else
		{
			LOG_WARNING("[MISSING]  " + paddedLabel + ": " + path.string());
		}
	};

	LOG_INFO("========== AssetSystem Configuration ==========");
	logPath("Working Directory", m_workingDirectory, true);
	logPath("Executable Directory", m_executableDirectory, true);
	logPath("Engine", m_enginePath, true);
	logPath("Engine Assets", m_engineAssetsPath, true);
	logPath("Project", m_projectPath, false);
	logPath("Project Assets", m_projectAssetsPath, false);
	logPath("Shader Symbols Output", m_shaderSymbolsOutputPath, false);
	LOG_INFO("================================================");
}

AssetSystem::~AssetSystem()
{
	m_projectPath.clear();
	m_projectAssetsPath.clear();
	m_enginePath.clear();
	m_engineAssetsPath.clear();
	m_workingDirectory.clear();
	m_executableDirectory.clear();

	for (auto& path : m_projectTypedPaths)
	{
		path.clear();
	}
	for (auto& path : m_engineTypedPaths)
	{
		path.clear();
	}

	m_shaderSymbolsOutputPath.clear();
}

const std::filesystem::path& AssetSystem::GetTypedPath(AssetType type, PathRoot root) const noexcept
{
	const size_t idx = static_cast<size_t>(type);
	if (type == AssetType::Count || idx >= kAssetTypeCount)
	{
		return s_emptyPath;
	}

	switch (root)
	{
		case PathRoot::Project:
			return m_projectTypedPaths[idx];

		case PathRoot::Engine:
			return m_engineTypedPaths[idx];

		case PathRoot::Any:
		default:
		{
			const auto& projectPath = m_projectTypedPaths[idx];
			return !projectPath.empty() ? projectPath : m_engineTypedPaths[idx];
		}
	}
}

std::optional<std::filesystem::path> AssetSystem::ResolvePath(const std::filesystem::path& inputPath, AssetType type) const
{
	if (inputPath.empty())
	{
		return std::nullopt;
	}

	if (inputPath.is_absolute())
	{
		std::error_code ec;
		return std::filesystem::exists(inputPath, ec) ? std::make_optional(inputPath) : std::nullopt;
	}

	if (auto result = TryResolveIn(m_projectAssetsPath, inputPath, type))
	{
		return result;
	}

	if (auto result = TryResolveIn(m_engineAssetsPath, inputPath, type))
	{
		return result;
	}

	return std::nullopt;
}

std::filesystem::path AssetSystem::ResolvePathValidated(const std::filesystem::path& inputPath, AssetType type) const
{
	if (auto resolved = ResolvePath(inputPath, type))
	{
		return *resolved;
	}

	LOG_FATAL(std::string(GetAssetTypeName(type)) + " asset not found: " + inputPath.string());
	return {};
}

std::optional<std::filesystem::path> AssetSystem::TryResolveIn(
    const std::filesystem::path& searchDir,
    const std::filesystem::path& relativePath,
    AssetType type) const
{
	if (searchDir.empty())
	{
		return std::nullopt;
	}

	std::error_code ec;

	if (const auto subdir = GetAssetSubdirectory(type); !subdir.empty())
	{
		auto candidate = searchDir / subdir / relativePath;
		if (std::filesystem::exists(candidate, ec))
		{
			return std::filesystem::weakly_canonical(candidate);
		}
	}

	auto candidate = searchDir / relativePath;
	if (std::filesystem::exists(candidate, ec))
	{
		return std::filesystem::weakly_canonical(candidate);
	}

	return std::nullopt;
}

const std::filesystem::path& AssetSystem::GetShaderPath(PathRoot root) const noexcept
{
	return GetTypedPath(AssetType::Shader, root);
}

const std::filesystem::path& AssetSystem::GetShaderSymbolsPath(PathRoot root) const noexcept
{
	return GetTypedPath(AssetType::ShaderSymbols, root);
}

const std::filesystem::path& AssetSystem::GetTexturePath(PathRoot root) const noexcept
{
	return GetTypedPath(AssetType::Texture, root);
}

const std::filesystem::path& AssetSystem::GetMeshPath(PathRoot root) const noexcept
{
	return GetTypedPath(AssetType::Mesh, root);
}