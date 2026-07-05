#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace Paths::Private
{
	std::string InferProjectNameFromExecutableStem(std::string_view executableStem);
	std::filesystem::path DefaultLogDirectory(bool ensureParentExists, std::string_view executableStem);
}
