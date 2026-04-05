#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace Engine::Paths
{
	std::string_view GetFileName(std::string_view path) noexcept;
	std::filesystem::path Normalize(const std::filesystem::path& path);
	std::wstring MakePathKey(const std::filesystem::path& path);
	std::wstring GetLowercaseExtension(const std::filesystem::path& path);
}