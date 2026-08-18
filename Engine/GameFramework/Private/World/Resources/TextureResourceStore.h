#pragma once

#include "GameFramework/Public/Scene/Materials/MaterialDesc.h"
#include "GameFramework/Public/Rendering/RenderResourceTables.h"

#include <filesystem>
#include <span>
#include <vector>
#include <cstdint>

class TextureResourceStore final
{
public:
	explicit TextureResourceStore(std::uint32_t generation) noexcept :
	    m_generation(generation)
	{
	}
	void AppendMaterialReferences(const std::vector<MaterialDesc>& materials);
	void AppendPaths(std::span<const std::filesystem::path> paths);
	std::uint64_t GetContentRevision() const noexcept { return m_contentRevision; }
	RenderTextureTable CaptureRenderTable() const;
	RenderTextureTable CaptureRenderTable(std::span<const std::filesystem::path> additionalPaths) const;

private:
	std::vector<std::filesystem::path> m_paths;
	std::uint32_t m_generation = 0;
	std::uint64_t m_contentRevision = 0;
};
