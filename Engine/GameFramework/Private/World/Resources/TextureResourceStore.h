#pragma once

#include "GameFramework/Public/Scene/Materials/MaterialDesc.h"
#include "GameFramework/Public/Scene/Textures/TextureSnapshot.h"

#include <filesystem>
#include <span>
#include <vector>
#include <cstdint>

class TextureResourceStore final
{
  public:
	explicit TextureResourceStore(std::uint32_t generation) noexcept : m_generation(generation) {}
	void AppendMaterialReferences(const std::vector<MaterialDesc>& materials);
	void AppendPaths(std::span<const std::filesystem::path> paths);
	TextureSnapshot CaptureSnapshot() const;
	TextureSnapshot CaptureSnapshot(std::span<const std::filesystem::path> additionalPaths) const;

  private:
	std::vector<std::filesystem::path> m_paths;
	std::uint32_t m_generation = 0;
};
