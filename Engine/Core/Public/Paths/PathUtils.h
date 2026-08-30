#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace Paths
{
	std::string_view GetFileName(std::string_view path) noexcept;
	std::string MakeProjectRelativeString(const std::filesystem::path& path);
	std::filesystem::path Normalize(const std::filesystem::path& path);
	std::optional<std::filesystem::path> ResolveRelativePath(const std::filesystem::path& baseDirectory, const std::filesystem::path& path);
	bool IsUnderRoot(const std::filesystem::path& path, const std::filesystem::path& root);
	std::optional<std::filesystem::path> TryMakeRelativeUnderRoot(const std::filesystem::path& path, const std::filesystem::path& root);
	std::string MakeSafePathComponent(std::string_view value);
	std::wstring MakePathKey(const std::filesystem::path& path);
	std::wstring GetLowercaseExtension(const std::filesystem::path& path);
}
