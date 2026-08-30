#pragma once

#include "ShaderRecook/ShaderRecookPublication.h"

#include <filesystem>
#include <string_view>

class ShaderRecookPublicationReader final
{
public:
	static ShaderRecookPublicationReadResult Read(const std::filesystem::path& publicationPath) noexcept;

private:
	static ShaderRecookPublicationReadResult Parse(std::string_view text) noexcept;
};
