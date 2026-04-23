#include "PCH.h"

#include "Cooking/Cache/LocalDiskShaderArtifactStore.h"

#include "Core/Public/Files/FileUtils.h"

#include <chrono>
#include <format>
#include <limits>

std::filesystem::path LocalDiskShaderArtifactStore::BuildArtifactPath(const ShaderCacheKey& key) const
{
	const std::string hex = key.ToHex();
	return m_rootDirectory / hex.substr(0, 2) / (hex + ".bin");
}

bool LocalDiskShaderArtifactStore::WriteString(
	std::vector<std::uint8_t>& outBytes,
	const std::string& value,
	std::string& outErrorMessage)
{
	if (value.size() > static_cast<std::size_t>((std::numeric_limits<std::uint32_t>::max)()))
	{
		outErrorMessage = "Cache serialization failed: string field too large";
		return false;
	}

	const std::uint32_t sizeInBytes = static_cast<std::uint32_t>(value.size());
	WritePOD(outBytes, sizeInBytes);
	outBytes.insert(outBytes.end(), value.begin(), value.end());
	return true;
}

bool LocalDiskShaderArtifactStore::ReadString(
	std::span<const std::uint8_t> bytes,
	std::size_t& cursor,
	std::string& outValue)
{
	std::uint32_t sizeInBytes = 0;
	if (!ReadPOD(bytes, cursor, sizeInBytes) || cursor + sizeInBytes > bytes.size())
	{
		return false;
	}

	outValue.assign(reinterpret_cast<const char*>(bytes.data() + cursor), sizeInBytes);
	cursor += sizeInBytes;
	return true;
}

bool LocalDiskShaderArtifactStore::Serialize(
	const CookedStageBuild& build,
	std::vector<std::uint8_t>& outBytes,
	std::string& outErrorMessage)
{
	outBytes.clear();
	outBytes.reserve(build.bytecode.size() + 256);

	WritePOD(outBytes, kFormatMagic);
	WritePOD(outBytes, kFormatVersion);
	WritePOD(outBytes, static_cast<std::uint16_t>(build.stage));
	WritePOD(outBytes, build.bytecodeHash);

	if (!WriteString(outBytes, build.sourcePath, outErrorMessage) || !WriteString(outBytes, build.entryPoint, outErrorMessage) ||
		!WriteString(outBytes, build.debugArtifact, outErrorMessage))
	{
		return false;
	}

	if (build.bytecode.size() > static_cast<std::size_t>((std::numeric_limits<std::uint32_t>::max)()))
	{
		outErrorMessage = "Cache serialization failed: bytecode too large";
		return false;
	}

	const std::uint32_t bytecodeSize = static_cast<std::uint32_t>(build.bytecode.size());
	WritePOD(outBytes, bytecodeSize);
	outBytes.insert(outBytes.end(), build.bytecode.begin(), build.bytecode.end());

	outErrorMessage.clear();
	return true;
}

bool LocalDiskShaderArtifactStore::Deserialize(
	std::span<const std::uint8_t> bytes,
	CookedStageBuild& outBuild,
	std::string& outErrorMessage)
{
	std::size_t cursor = 0;
	std::uint32_t magic = 0;
	std::uint32_t version = 0;
	std::uint16_t stage = 0;
	std::uint64_t bytecodeHash = 0;

	if (!ReadPOD(bytes, cursor, magic) || !ReadPOD(bytes, cursor, version) || !ReadPOD(bytes, cursor, stage) ||
		!ReadPOD(bytes, cursor, bytecodeHash))
	{
		outErrorMessage = "Cache deserialize failed: truncated header";
		return false;
	}

	if (magic != kFormatMagic || version != kFormatVersion)
	{
		outErrorMessage = "Cache deserialize failed: unsupported artifact format";
		return false;
	}

	CookedStageBuild build;
	build.stage = static_cast<ShaderStage>(stage);
	build.bytecodeHash = bytecodeHash;

	if (!ReadString(bytes, cursor, build.sourcePath) || !ReadString(bytes, cursor, build.entryPoint) ||
		!ReadString(bytes, cursor, build.debugArtifact))
	{
		outErrorMessage = "Cache deserialize failed: truncated string payload";
		return false;
	}

	std::uint32_t bytecodeSize = 0;
	if (!ReadPOD(bytes, cursor, bytecodeSize) || cursor + bytecodeSize > bytes.size())
	{
		outErrorMessage = "Cache deserialize failed: truncated bytecode payload";
		return false;
	}

	build.bytecode.assign(bytes.begin() + static_cast<std::ptrdiff_t>(cursor), bytes.begin() + static_cast<std::ptrdiff_t>(cursor + bytecodeSize));
	outBuild = std::move(build);
	outErrorMessage.clear();
	return true;
}

bool LocalDiskShaderArtifactStore::TryGet(
	const ShaderCacheKey& key,
	CookedStageBuild& outBuild,
	std::string& outErrorMessage) const
{
	std::vector<std::uint8_t> bytes;
	const std::filesystem::path artifactPath = BuildArtifactPath(key);
	if (!std::filesystem::exists(artifactPath))
	{
		outErrorMessage.clear();
		return false;
	}

	if (!Engine::Files::TryReadAllBytes(artifactPath, bytes, outErrorMessage))
	{
		return false;
	}

	return Deserialize(bytes, outBuild, outErrorMessage);
}

bool LocalDiskShaderArtifactStore::Put(
	const ShaderCacheKey& key,
	const CookedStageBuild& build,
	std::string& outErrorMessage)
{
	std::vector<std::uint8_t> bytes;
	if (!Serialize(build, bytes, outErrorMessage))
	{
		return false;
	}

	const std::filesystem::path artifactPath = BuildArtifactPath(key);
	std::error_code ec;
	if (artifactPath.has_parent_path())
	{
		std::filesystem::create_directories(artifactPath.parent_path(), ec);
		if (ec)
		{
			outErrorMessage = std::format("Failed to create cache directory '{}' - {}", artifactPath.parent_path().string(), ec.message());
			return false;
		}
	}

	const auto nowTicks = std::chrono::steady_clock::now().time_since_epoch().count();
	const std::filesystem::path tempPath = artifactPath.string() + "." + std::to_string(nowTicks) + ".tmp";
	if (!Engine::Files::TryWriteAllBytes(tempPath, bytes, outErrorMessage))
	{
		return false;
	}

	ec.clear();
	if (std::filesystem::exists(artifactPath, ec) && !ec)
	{
		std::filesystem::remove(tempPath, ec);
		outErrorMessage.clear();
		return true;
	}

	ec.clear();
	std::filesystem::rename(tempPath, artifactPath, ec);
	if (ec)
	{
		std::filesystem::remove(tempPath, ec);
		outErrorMessage = std::format("Failed to move cache artifact '{}' into place - {}", artifactPath.string(), ec.message());
		return false;
	}

	outErrorMessage.clear();
	return true;
}
