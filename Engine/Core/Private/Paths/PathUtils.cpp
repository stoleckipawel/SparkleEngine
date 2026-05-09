#include "PCH.h"

#include "Core/Public/Paths/PathUtils.h"

#include "Core/Public/Paths/DirectoryPaths.h"

#include <algorithm>
#include <cwctype>
#include <system_error>

namespace Paths
{
	bool IsUnderRoot(const std::filesystem::path& path, const std::filesystem::path& root)
	{
		if (path.empty() || root.empty())
		{
			return false;
		}

		std::error_code ec;
		const std::filesystem::path relativePath = std::filesystem::relative(path, root, ec);
		if (ec)
		{
			return false;
		}

		const std::string relativePathString = relativePath.generic_string();
		return !relativePathString.empty() && !relativePathString.starts_with("..");
	}

	std::optional<std::filesystem::path> TryMakeRelativeUnderRoot(const std::filesystem::path& path, const std::filesystem::path& root)
	{
		if (!IsUnderRoot(path, root))
		{
			return std::nullopt;
		}

		std::error_code ec;
		const std::filesystem::path relativePath = std::filesystem::relative(path, root, ec);
		if (ec || relativePath.empty())
		{
			return std::nullopt;
		}

		return relativePath;
	}

	std::string_view GetFileName(std::string_view path) noexcept
	{
		for (std::size_t index = path.size(); index > 0; --index)
		{
			if (path[index - 1] == '/' || path[index - 1] == '\\')
			{
				return path.substr(index);
			}
		}

		return path;
	}

	std::string MakeProjectRelativeString(const std::filesystem::path& path)
	{
		if (path.empty())
		{
			return {};
		}

		const std::filesystem::path normalizedPath = Normalize(path);
		const std::filesystem::path& projectRoot = Paths::ProjectRoot();
		if (IsUnderRoot(normalizedPath, projectRoot))
		{
			std::error_code ec;
			const std::filesystem::path relativePath = std::filesystem::relative(normalizedPath, projectRoot, ec);
			if (!ec)
			{
				return relativePath.generic_string();
			}
		}

		return normalizedPath.generic_string();
	}

	std::filesystem::path Normalize(const std::filesystem::path& path)
	{
		if (path.empty())
		{
			return {};
		}

		auto normalized = path.is_relative() ? std::filesystem::absolute(path) : path;
		normalized.make_preferred();

		std::error_code ec;
		if (auto canonical = std::filesystem::weakly_canonical(normalized, ec); !ec)
		{
			return canonical;
		}
		return normalized;
	}

	std::optional<std::filesystem::path> ResolveRelativePath(const std::filesystem::path& baseDirectory, const std::filesystem::path& path)
	{
		if (path.empty())
		{
			return std::nullopt;
		}

		std::filesystem::path resolvedPath = path;
		if (!resolvedPath.is_absolute())
		{
			resolvedPath = baseDirectory / resolvedPath;
		}

		resolvedPath = Normalize(resolvedPath);
		if (resolvedPath.empty())
		{
			return std::nullopt;
		}

		return resolvedPath;
	}

	std::string MakeSafePathComponent(std::string_view value)
	{
		std::string result;
		result.reserve(value.size());
		for (const char character : value)
		{
			const bool allowed = (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
			                     (character >= '0' && character <= '9') || character == '_' || character == '-' || character == '.';
			result.push_back(allowed ? character : '_');
		}
		return result;
	}

	std::wstring MakePathKey(const std::filesystem::path& path)
	{
		const std::filesystem::path normalizedPath = Normalize(path);
		if (normalizedPath.empty())
		{
			return {};
		}

		std::wstring key = normalizedPath.generic_wstring();
		std::transform(
		    key.begin(),
		    key.end(),
		    key.begin(),
		    [](wchar_t value)
		    {
			    return static_cast<wchar_t>(std::towlower(value));
		    });
		return key;
	}

	std::wstring GetLowercaseExtension(const std::filesystem::path& path)
	{
		std::wstring extension = path.extension().wstring();
		std::transform(
		    extension.begin(),
		    extension.end(),
		    extension.begin(),
		    [](wchar_t value)
		    {
			    return static_cast<wchar_t>(std::towlower(value));
		    });
		return extension;
	}
}