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

	bool TryGet(const ShaderCacheKey& key, CookedStageBuild& outBuild, std::string& outErrorMessage) const override;
	bool Put(const ShaderCacheKey& key, const CookedStageBuild& build, std::string& outErrorMessage) override;

  private:
	static constexpr std::uint32_t kFormatMagic = 0x31414353;
	static constexpr std::uint32_t kFormatVersion = 1;

	std::filesystem::path BuildArtifactPath(const ShaderCacheKey& key) const;
	static bool Serialize(const CookedStageBuild& build, std::vector<std::uint8_t>& outBytes, std::string& outErrorMessage);
	static bool Deserialize(std::span<const std::uint8_t> bytes, CookedStageBuild& outBuild, std::string& outErrorMessage);
	static bool SerializeReflection(
	    const struct ShaderReflection& reflection,
	    Files::BinaryBufferWriter& writer,
	    std::string& outErrorMessage);
	static bool DeserializeReflection(
	    Files::BinarySpanReader& reader,
	    struct ShaderReflection& outReflection,
	    std::string& outErrorMessage);

	std::filesystem::path m_rootDirectory;
};
