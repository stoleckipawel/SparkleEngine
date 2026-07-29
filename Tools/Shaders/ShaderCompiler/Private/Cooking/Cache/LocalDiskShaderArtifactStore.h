#pragma once

#include "Cooking/Cache/IShaderArtifactStore.h"
#include "Core/Public/Files/BinaryBufferWriter.h"
#include "Core/Public/Files/BinarySpanReader.h"

#include <filesystem>
#include <vector>

class LocalDiskShaderArtifactStore final : public IShaderArtifactStore
{
  public:
	explicit LocalDiskShaderArtifactStore(std::filesystem::path rootDirectory)
		: m_rootDirectory(std::move(rootDirectory))
	{
	}

	std::optional<CookedStageBuild> Find(const ShaderCacheKey& key) const override;
	void Put(const ShaderCacheKey& key, const CookedStageBuild& build) override;

  private:
	static constexpr std::uint32_t kFormatMagic = 0x31414353;
	static constexpr std::uint32_t kFormatVersion = 1;

	std::filesystem::path BuildArtifactPath(const ShaderCacheKey& key) const;
	static std::vector<std::uint8_t> Serialize(const CookedStageBuild& build);
	static CookedStageBuild Deserialize(std::span<const std::uint8_t> bytes);
	static void SerializeReflection(
	    const struct ShaderReflection& reflection,
	    Files::BinaryBufferWriter& writer);
	static void DeserializeReflection(
	    Files::BinarySpanReader& reader,
	    struct ShaderReflection& outReflection);

	std::filesystem::path m_rootDirectory;
};
