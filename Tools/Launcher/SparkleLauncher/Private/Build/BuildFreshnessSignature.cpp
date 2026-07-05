#include "BuildFreshnessSignature.h"

#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Core/Public/Strings/StringUtils.h"

#include <algorithm>
#include <array>
#include <sstream>
#include <string_view>
#include <system_error>

namespace SparkleLauncher
{
	static bool IsBuildInputFile(const std::filesystem::path& path)
	{
		const std::string fileName = path.filename().string();
		const std::string extension = path.extension().string();
		return fileName == "CMakeLists.txt" || fileName == Filesystem::kProjectMarker || extension == ".cmake";
	}

	static bool IsSourceFile(const std::filesystem::path& path)
	{
		static const std::array<std::string_view, 9> extensions = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".inl"};
		const std::string extension = Strings::ToLowerCopy(path.extension().string());
		return std::any_of(extensions.begin(), extensions.end(), [&extension](std::string_view candidate) {
			return extension == candidate;
		});
	}

	static bool IsUnderThirdParty(const std::filesystem::path& path)
	{
		const std::string normalized = Strings::ToLowerCopy(path.generic_string());
		return normalized.find("/third_party/") != std::string::npos || normalized.find("\\third_party\\") != std::string::npos;
	}

	static std::string MakeFreshnessSignaturePath(const std::filesystem::path& repositoryRoot, const std::filesystem::path& path)
	{
		const std::optional<std::filesystem::path> relativePath = Paths::TryMakeRelativeUnderRoot(path, repositoryRoot);
		std::string value = relativePath.has_value() ? relativePath->generic_string() : path.filename().generic_string();
		std::replace(value.begin(), value.end(), '/', '\\');
		return Strings::ToLowerCopy(value);
	}

	std::vector<std::filesystem::path> CollectBuildInputPaths(const std::filesystem::path& repositoryRoot)
	{
		std::vector<std::filesystem::path> paths;
		const std::filesystem::path rootCMake = repositoryRoot / "CMakeLists.txt";
		std::error_code errorCode;
		if (std::filesystem::is_regular_file(rootCMake, errorCode))
		{
			paths.push_back(rootCMake);
		}

		static const std::array<std::string_view, 4> directories = {"CMake", "Engine", "Tools", "Projects"};
		for (std::string_view directory : directories)
		{
			const std::filesystem::path root = repositoryRoot / std::string(directory);
			if (!std::filesystem::is_directory(root, errorCode))
			{
				continue;
			}

			std::filesystem::recursive_directory_iterator iterator(root, std::filesystem::directory_options::skip_permission_denied, errorCode);
			const std::filesystem::recursive_directory_iterator endIterator;
			while (iterator != endIterator)
			{
				const std::filesystem::directory_entry entry = *iterator;
				if (entry.is_regular_file(errorCode) && IsBuildInputFile(entry.path()))
				{
					paths.push_back(entry.path());
				}
				iterator.increment(errorCode);
				errorCode.clear();
			}
		}

		return paths;
	}

	std::optional<std::string> ComputeSourceListHash(const std::filesystem::path& repositoryRoot)
	{
		std::vector<std::string> relativePaths;
		static const std::array<std::string_view, 3> directories = {"Engine", "Tools", "Projects"};
		for (std::string_view directory : directories)
		{
			const std::filesystem::path root = repositoryRoot / std::string(directory);
			std::error_code errorCode;
			if (!std::filesystem::is_directory(root, errorCode))
			{
				continue;
			}

			std::filesystem::recursive_directory_iterator iterator(root, std::filesystem::directory_options::skip_permission_denied, errorCode);
			const std::filesystem::recursive_directory_iterator endIterator;
			while (iterator != endIterator)
			{
				const std::filesystem::directory_entry entry = *iterator;
				if (entry.is_regular_file(errorCode) && IsSourceFile(entry.path()) && !IsUnderThirdParty(entry.path()))
				{
					relativePaths.push_back(MakeFreshnessSignaturePath(repositoryRoot, entry.path()));
				}
				iterator.increment(errorCode);
				errorCode.clear();
			}
		}

		std::sort(relativePaths.begin(), relativePaths.end());
		std::ostringstream signature;
		for (std::size_t index = 0; index < relativePaths.size(); ++index)
		{
			if (index > 0)
			{
				signature << '\n';
			}
			signature << relativePaths[index];
		}

		std::string hash;
		std::string errorMessage;
		return Hash::TrySha256Hex(signature.str(), hash, errorMessage) ? std::optional<std::string>(hash) : std::nullopt;
	}
}
