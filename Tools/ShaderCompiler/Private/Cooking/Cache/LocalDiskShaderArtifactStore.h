#pragma once

#include "Cooking/Cache/IShaderArtifactStore.h"

#include <cstring>
#include <filesystem>
#include <span>
#include <type_traits>
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

	template <typename T>
	static void WritePOD(std::vector<std::uint8_t>& outBytes, const T& value)
	{
		static_assert(std::is_trivially_copyable_v<T>);
		const auto* begin = reinterpret_cast<const std::uint8_t*>(&value);
		outBytes.insert(outBytes.end(), begin, begin + sizeof(T));
	}

	template <typename T>
	static bool ReadPOD(std::span<const std::uint8_t> bytes, std::size_t& cursor, T& outValue)
	{
		if (cursor + sizeof(T) > bytes.size())
		{
			return false;
		}

		std::memcpy(&outValue, bytes.data() + cursor, sizeof(T));
		cursor += sizeof(T);
		return true;
	}

	std::filesystem::path BuildArtifactPath(const ShaderCacheKey& key) const;
	static bool WriteString(std::vector<std::uint8_t>& outBytes, const std::string& value, std::string& outErrorMessage);
	static bool ReadString(std::span<const std::uint8_t> bytes, std::size_t& cursor, std::string& outValue);
	static bool Serialize(const CookedStageBuild& build, std::vector<std::uint8_t>& outBytes, std::string& outErrorMessage);
	static bool Deserialize(std::span<const std::uint8_t> bytes, CookedStageBuild& outBuild, std::string& outErrorMessage);

	std::filesystem::path m_rootDirectory;
};
