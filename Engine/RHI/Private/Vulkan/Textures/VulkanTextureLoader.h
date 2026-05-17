#pragma once

#include "Resources/Texture.h"

#include <filesystem>
#include <memory>

class VulkanTextureLoader final
{
  public:
	static std::unique_ptr<Texture> Load(const std::filesystem::path& texturePath);

	VulkanTextureLoader() = delete;
	~VulkanTextureLoader() = delete;
};