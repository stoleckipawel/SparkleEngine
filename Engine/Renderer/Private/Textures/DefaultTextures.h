#pragma once

#include "Core/Public/Assets/DefaultTexture.h"

#include <filesystem>

namespace DefaultTextures
{
	const DefaultTextureDesc& GetDesc(DefaultTexture type);
	const char* GetName(DefaultTexture type);
	std::filesystem::path GetPath(DefaultTexture type);
}
