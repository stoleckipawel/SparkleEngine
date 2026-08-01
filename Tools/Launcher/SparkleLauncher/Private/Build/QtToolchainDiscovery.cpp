#include "QtToolchainDiscovery.h"

#include "Core/Public/Environment/EnvironmentVariables.h"

#include <algorithm>
#include <cctype>
#include <optional>
#include <ranges>
#include <sstream>
#include <system_error>

namespace SparkleLauncher
{
	static std::string ToLower(std::string value)
	{
		std::transform(
		    value.begin(),
		    value.end(),
		    value.begin(),
		    [](unsigned char character)
		    {
			    return static_cast<char>(std::tolower(character));
		    });
		return value;
	}

	static std::string BuildPathSortKey(const std::filesystem::path& path)
	{
		return ToLower(path.lexically_normal().generic_string());
	}

	static bool PathLooksLikeMsvcKitRoot(const std::filesystem::path& path)
	{
		const std::string directoryName = ToLower(path.filename().string());
		return directoryName.find("msvc") != std::string::npos && directoryName.find("64") != std::string::npos &&
		       directoryName.find("arm64") == std::string::npos;
	}

	static bool PathLooksLikeMingwKitRoot(const std::filesystem::path& path)
	{
		return ToLower(path.filename().string()).find("mingw") != std::string::npos;
	}

	static std::optional<std::filesystem::path> NormalizeKitRootCandidate(std::filesystem::path path)
	{
		std::error_code errorCode;
		if (path.empty())
		{
			return std::nullopt;
		}

		path = path.lexically_normal();
		if (std::filesystem::is_regular_file(path, errorCode))
		{
			if (ToLower(path.filename().string()) == "qmake.exe")
			{
				const std::filesystem::path root = path.parent_path().parent_path();
				return std::filesystem::exists(root / "lib" / "cmake" / "Qt6" / "Qt6Config.cmake", errorCode)
				           ? std::optional<std::filesystem::path>(root)
				           : std::nullopt;
			}
			return std::nullopt;
		}
		errorCode.clear();

		if (!std::filesystem::is_directory(path, errorCode))
		{
			return std::nullopt;
		}
		errorCode.clear();

		if (std::filesystem::exists(path / "lib" / "cmake" / "Qt6" / "Qt6Config.cmake", errorCode))
		{
			return path;
		}
		errorCode.clear();

		if (ToLower(path.filename().string()) == "qt6")
		{
			const std::filesystem::path root = path.parent_path().parent_path();
			if (std::filesystem::exists(root / "bin" / "qmake.exe", errorCode) &&
			    std::filesystem::exists(root / "lib" / "cmake" / "Qt6" / "Qt6Config.cmake", errorCode))
			{
				return root;
			}
		}
		return std::nullopt;
	}

	static std::vector<std::filesystem::path> SplitPathList(std::string_view value)
	{
		std::vector<std::filesystem::path> paths;
		std::stringstream stream{std::string(value)};
		std::string segment;
		while (std::getline(stream, segment, ';'))
		{
			if (!segment.empty())
			{
				paths.emplace_back(segment);
			}
		}
		return paths;
	}

	static void AddKitCandidate(
	    QtToolchainDiscovery& discovery,
	    const std::filesystem::path& candidate,
	    std::vector<std::filesystem::path>& seenCandidates)
	{
		const std::optional<std::filesystem::path> normalizedRoot = NormalizeKitRootCandidate(candidate);
		if (!normalizedRoot.has_value())
		{
			return;
		}

		const std::filesystem::path root = normalizedRoot->lexically_normal();
		if (std::find(seenCandidates.begin(), seenCandidates.end(), root) != seenCandidates.end())
		{
			return;
		}
		seenCandidates.push_back(root);

		if (PathLooksLikeMsvcKitRoot(root))
		{
			discovery.MsvcCandidates.push_back(root);
		}
		else if (PathLooksLikeMingwKitRoot(root))
		{
			discovery.MingwCandidates.push_back(root);
		}
	}

	static void FinalizeDiscovery(QtToolchainDiscovery& discovery)
	{
		const auto byPath = [](const std::filesystem::path& left, const std::filesystem::path& right)
		{
			return BuildPathSortKey(left) < BuildPathSortKey(right);
		};
		std::ranges::sort(discovery.MsvcCandidates, byPath);
		std::ranges::sort(discovery.MingwCandidates, byPath);

		discovery.FoundMsvcKit = !discovery.MsvcCandidates.empty();
		discovery.QtRootPath = discovery.FoundMsvcKit ? discovery.MsvcCandidates.front() : std::filesystem::path();
		discovery.QtQmakePath = discovery.FoundMsvcKit ? discovery.QtRootPath / "bin" / "qmake.exe" : std::filesystem::path();
	}

	QtToolchainDiscovery DiscoverQtToolchain()
	{
		QtToolchainDiscovery discovery;
		std::vector<std::filesystem::path> seenCandidates;
		std::string value;
		if (Environment::TryGetVariable("SPARKLE_QT_ROOT", value))
		{
			AddKitCandidate(discovery, value, seenCandidates);
		}
		if (Environment::TryGetVariable("QTDIR", value))
		{
			AddKitCandidate(discovery, value, seenCandidates);
		}
		if (Environment::TryGetVariable("Qt6_DIR", value))
		{
			AddKitCandidate(discovery, value, seenCandidates);
		}
		if (Environment::TryGetVariable("CMAKE_PREFIX_PATH", value))
		{
			for (const std::filesystem::path& path : SplitPathList(value))
			{
				AddKitCandidate(discovery, path, seenCandidates);
			}
		}

		const std::filesystem::path qtRoot("C:\\Qt");
		std::error_code errorCode;
		if (std::filesystem::is_directory(qtRoot, errorCode))
		{
			for (const std::filesystem::directory_entry& versionEntry : std::filesystem::directory_iterator(qtRoot, errorCode))
			{
				if (errorCode || !versionEntry.is_directory(errorCode))
				{
					errorCode.clear();
					continue;
				}
				for (const std::filesystem::directory_entry& kitEntry : std::filesystem::directory_iterator(versionEntry.path(), errorCode))
				{
					if (errorCode)
					{
						errorCode.clear();
						continue;
					}
					AddKitCandidate(discovery, kitEntry.path(), seenCandidates);
				}
			}
		}

		FinalizeDiscovery(discovery);
		return discovery;
	}

	std::string BuildQtToolchainStatusDetail(const QtToolchainDiscovery& discovery)
	{
		if (discovery.FoundMsvcKit)
		{
			return "Using Qt MSVC kit root: " + discovery.QtRootPath.string();
		}
		if (!discovery.MingwCandidates.empty())
		{
			return "Only MinGW Qt kits were found. Install a Qt 6 MSVC x64 kit and set SPARKLE_QT_ROOT if needed.";
		}
		return "No Qt 6 MSVC x64 kit was found. Install one under C:\\Qt or set SPARKLE_QT_ROOT to the kit root.";
	}
}
