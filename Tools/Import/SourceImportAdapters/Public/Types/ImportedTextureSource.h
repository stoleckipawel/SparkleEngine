#pragma once

#include "Core/Public/Assets/TextureGroup.h"
#include "Core/Public/Assets/TextureProperties.h"

#include <filesystem>

struct ImportedTextureSource
{
	TextureGroup textureGroup = TextureGroup::Default;
	std::filesystem::path sourcePath;
	TextureChannelMask channelMask = TextureChannelMask::Rgba;
};
