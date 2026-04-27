#include "PCH.h"

#include "Cooking/Cache/LocalDiskShaderArtifactStore.h"

#include "Core/Public/Files/FileUtils.h"
#include "ShaderReflection.h"

#include <chrono>
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
	WritePOD(outBytes, static_cast<std::uint8_t>(build.format));
	WritePOD(outBytes, build.bytecodeHash);
	WritePOD(outBytes, build.backendVersion);

	if (!WriteString(outBytes, build.sourcePath, outErrorMessage) ||
		!WriteString(outBytes, build.entryPoint, outErrorMessage) ||
		!WriteString(outBytes, build.debugArtifact, outErrorMessage) ||
		!WriteString(outBytes, build.backendName, outErrorMessage))
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

	if (!SerializeReflection(build.reflection, outBytes, outErrorMessage))
	{
		return false;
	}

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
	std::uint8_t format = 0;
	std::uint64_t bytecodeHash = 0;
	std::uint64_t backendVersion = 0;

	if (!ReadPOD(bytes, cursor, magic) || !ReadPOD(bytes, cursor, version) ||
		!ReadPOD(bytes, cursor, stage) || !ReadPOD(bytes, cursor, format) ||
		!ReadPOD(bytes, cursor, bytecodeHash) || !ReadPOD(bytes, cursor, backendVersion))
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
	build.format = static_cast<CookedShaderBinaryFormat>(format);
	build.bytecodeHash = bytecodeHash;
	build.backendVersion = backendVersion;

	if (!ReadString(bytes, cursor, build.sourcePath) || !ReadString(bytes, cursor, build.entryPoint) ||
		!ReadString(bytes, cursor, build.debugArtifact) || !ReadString(bytes, cursor, build.backendName))
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
	cursor += bytecodeSize;

	if (!DeserializeReflection(bytes, cursor, build.reflection, outErrorMessage))
	{
		return false;
	}

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

	if (!Files::TryReadAllBytes(artifactPath, bytes, outErrorMessage))
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
	const auto nowTicks = std::chrono::steady_clock::now().time_since_epoch().count();
	const std::filesystem::path tempPath = Files::BuildTemporaryPath(artifactPath, "." + std::to_string(nowTicks) + ".tmp");
	if (!Files::TryWriteAllBytes(tempPath, bytes, outErrorMessage))
	{
		return false;
	}

	if (!Files::TryFinalizeTemporaryFileIfMissing(tempPath, artifactPath, outErrorMessage))
	{
		return false;
	}

	outErrorMessage.clear();
	return true;
}

bool LocalDiskShaderArtifactStore::SerializeReflection(
    const ShaderReflection& reflection,
    std::vector<std::uint8_t>& outBytes,
    std::string& outErrorMessage)
{
	WritePOD(outBytes, reflection.ThreadGroupSize[0]);
	WritePOD(outBytes, reflection.ThreadGroupSize[1]);
	WritePOD(outBytes, reflection.ThreadGroupSize[2]);
	WritePOD(outBytes, reflection.EntryFlags);
	WritePOD(outBytes, reflection.WaveSize);

	WritePOD(outBytes, static_cast<std::uint32_t>(reflection.Bindings.size()));
	for (const ShaderReflectionResourceBinding& b : reflection.Bindings)
	{
		if (!WriteString(outBytes, b.Name, outErrorMessage))
		{
			return false;
		}
		WritePOD(outBytes, b.Kind);
		WritePOD(outBytes, b.Dimension);
		WritePOD(outBytes, static_cast<std::uint8_t>(b.IsReadOnly ? 1 : 0));
		WritePOD(outBytes, b.Set);
		WritePOD(outBytes, b.Slot);
		WritePOD(outBytes, b.ArrayCount);
		WritePOD(outBytes, b.SizeInBytes);
		WritePOD(outBytes, b.ConstantBufferIndex);
	}

	WritePOD(outBytes, static_cast<std::uint32_t>(reflection.ConstantBuffers.size()));
	for (const ShaderReflectionConstantBuffer& cb : reflection.ConstantBuffers)
	{
		if (!WriteString(outBytes, cb.Name, outErrorMessage))
		{
			return false;
		}
		WritePOD(outBytes, cb.SizeInBytes);
		WritePOD(outBytes, static_cast<std::uint32_t>(cb.Members.size()));
		for (const ShaderReflectionConstantBufferMember& m : cb.Members)
		{
			if (!WriteString(outBytes, m.Name, outErrorMessage))
			{
				return false;
			}
			WritePOD(outBytes, m.OffsetInBytes);
			WritePOD(outBytes, m.SizeInBytes);
			WritePOD(outBytes, m.ArrayCount);
			WritePOD(outBytes, m.ArrayStrideInBytes);
			WritePOD(outBytes, m.ScalarType);
			WritePOD(outBytes, m.RowCount);
			WritePOD(outBytes, m.ColumnCount);
		}
	}

	WritePOD(outBytes, static_cast<std::uint32_t>(reflection.InputElements.size()));
	for (const ShaderReflectionInputElement& e : reflection.InputElements)
	{
		if (!WriteString(outBytes, e.Semantic, outErrorMessage))
		{
			return false;
		}
		WritePOD(outBytes, e.SemanticIndex);
		WritePOD(outBytes, e.Location);
		WritePOD(outBytes, e.ScalarType);
		WritePOD(outBytes, e.ComponentCount);
	}

	WritePOD(outBytes, static_cast<std::uint32_t>(reflection.PushConstants.size()));
	for (const ShaderReflectionPushConstantRange& r : reflection.PushConstants)
	{
		WritePOD(outBytes, r.OffsetInBytes);
		WritePOD(outBytes, r.SizeInBytes);
		WritePOD(outBytes, r.VisibilityMask);
	}

	WritePOD(outBytes, static_cast<std::uint32_t>(reflection.SpecializationConstants.size()));
	for (const ShaderReflectionSpecializationConstant& s : reflection.SpecializationConstants)
	{
		if (!WriteString(outBytes, s.Name, outErrorMessage))
		{
			return false;
		}
		WritePOD(outBytes, s.ConstantId);
		WritePOD(outBytes, s.DefaultValueBits);
		WritePOD(outBytes, s.ScalarType);
	}

	return true;
}

bool LocalDiskShaderArtifactStore::DeserializeReflection(
    std::span<const std::uint8_t> bytes,
    std::size_t& cursor,
    ShaderReflection& outReflection,
    std::string& outErrorMessage)
{
	auto fail = [&](const char* msg) {
		outErrorMessage = msg;
		return false;
	};

	if (!ReadPOD(bytes, cursor, outReflection.ThreadGroupSize[0]) ||
	    !ReadPOD(bytes, cursor, outReflection.ThreadGroupSize[1]) ||
	    !ReadPOD(bytes, cursor, outReflection.ThreadGroupSize[2]) || !ReadPOD(bytes, cursor, outReflection.EntryFlags) ||
	    !ReadPOD(bytes, cursor, outReflection.WaveSize))
	{
		return fail("Cache deserialize failed: truncated reflection header");
	}

	std::uint32_t bindingCount = 0;
	if (!ReadPOD(bytes, cursor, bindingCount))
	{
		return fail("Cache deserialize failed: truncated bindings count");
	}
	outReflection.Bindings.resize(bindingCount);
	for (std::uint32_t i = 0; i < bindingCount; ++i)
	{
		ShaderReflectionResourceBinding& b = outReflection.Bindings[i];
		std::uint8_t isReadOnly = 0;
		if (!ReadString(bytes, cursor, b.Name) || !ReadPOD(bytes, cursor, b.Kind) || !ReadPOD(bytes, cursor, b.Dimension) ||
		    !ReadPOD(bytes, cursor, isReadOnly) || !ReadPOD(bytes, cursor, b.Set) || !ReadPOD(bytes, cursor, b.Slot) ||
		    !ReadPOD(bytes, cursor, b.ArrayCount) || !ReadPOD(bytes, cursor, b.SizeInBytes) ||
		    !ReadPOD(bytes, cursor, b.ConstantBufferIndex))
		{
			return fail("Cache deserialize failed: truncated binding record");
		}
		b.IsReadOnly = isReadOnly != 0;
	}

	std::uint32_t cbCount = 0;
	if (!ReadPOD(bytes, cursor, cbCount))
	{
		return fail("Cache deserialize failed: truncated CB count");
	}
	outReflection.ConstantBuffers.resize(cbCount);
	for (std::uint32_t i = 0; i < cbCount; ++i)
	{
		ShaderReflectionConstantBuffer& cb = outReflection.ConstantBuffers[i];
		std::uint32_t memberCount = 0;
		if (!ReadString(bytes, cursor, cb.Name) || !ReadPOD(bytes, cursor, cb.SizeInBytes) || !ReadPOD(bytes, cursor, memberCount))
		{
			return fail("Cache deserialize failed: truncated CB record");
		}
		cb.Members.resize(memberCount);
		for (std::uint32_t j = 0; j < memberCount; ++j)
		{
			ShaderReflectionConstantBufferMember& m = cb.Members[j];
			if (!ReadString(bytes, cursor, m.Name) || !ReadPOD(bytes, cursor, m.OffsetInBytes) ||
			    !ReadPOD(bytes, cursor, m.SizeInBytes) || !ReadPOD(bytes, cursor, m.ArrayCount) ||
			    !ReadPOD(bytes, cursor, m.ArrayStrideInBytes) || !ReadPOD(bytes, cursor, m.ScalarType) ||
			    !ReadPOD(bytes, cursor, m.RowCount) || !ReadPOD(bytes, cursor, m.ColumnCount))
			{
				return fail("Cache deserialize failed: truncated CB member");
			}
		}
	}

	std::uint32_t inputCount = 0;
	if (!ReadPOD(bytes, cursor, inputCount))
	{
		return fail("Cache deserialize failed: truncated input count");
	}
	outReflection.InputElements.resize(inputCount);
	for (std::uint32_t i = 0; i < inputCount; ++i)
	{
		ShaderReflectionInputElement& e = outReflection.InputElements[i];
		if (!ReadString(bytes, cursor, e.Semantic) || !ReadPOD(bytes, cursor, e.SemanticIndex) || !ReadPOD(bytes, cursor, e.Location) ||
		    !ReadPOD(bytes, cursor, e.ScalarType) || !ReadPOD(bytes, cursor, e.ComponentCount))
		{
			return fail("Cache deserialize failed: truncated input element");
		}
	}

	std::uint32_t pushCount = 0;
	if (!ReadPOD(bytes, cursor, pushCount))
	{
		return fail("Cache deserialize failed: truncated push count");
	}
	outReflection.PushConstants.resize(pushCount);
	for (std::uint32_t i = 0; i < pushCount; ++i)
	{
		ShaderReflectionPushConstantRange& r = outReflection.PushConstants[i];
		if (!ReadPOD(bytes, cursor, r.OffsetInBytes) || !ReadPOD(bytes, cursor, r.SizeInBytes) ||
		    !ReadPOD(bytes, cursor, r.VisibilityMask))
		{
			return fail("Cache deserialize failed: truncated push range");
		}
	}

	std::uint32_t specCount = 0;
	if (!ReadPOD(bytes, cursor, specCount))
	{
		return fail("Cache deserialize failed: truncated spec count");
	}
	outReflection.SpecializationConstants.resize(specCount);
	for (std::uint32_t i = 0; i < specCount; ++i)
	{
		ShaderReflectionSpecializationConstant& s = outReflection.SpecializationConstants[i];
		if (!ReadString(bytes, cursor, s.Name) || !ReadPOD(bytes, cursor, s.ConstantId) ||
		    !ReadPOD(bytes, cursor, s.DefaultValueBits) || !ReadPOD(bytes, cursor, s.ScalarType))
		{
			return fail("Cache deserialize failed: truncated spec constant");
		}
	}

	return true;
}