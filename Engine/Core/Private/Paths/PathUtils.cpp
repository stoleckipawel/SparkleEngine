#include "PCH.h"

#include "Core/Public/Paths/PathUtils.h"

#include <algorithm>
#include <cwctype>
#include <system_error>

namespace Engine::Paths
{
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