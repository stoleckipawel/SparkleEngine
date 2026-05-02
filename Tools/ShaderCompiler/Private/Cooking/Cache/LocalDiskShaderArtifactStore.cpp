#include "PCH.h"

#include "Cooking/Cache/LocalDiskShaderArtifactStore.h"

#include "Core/Public/Files/FileUtils.h"
#include "ShaderReflection.h"

#include <chrono>

std::filesystem::path LocalDiskShaderArtifactStore::BuildArtifactPath(const ShaderCacheKey& key) const
{
	const std::string hex = key.ToHex();
	return m_rootDirectory / hex.substr(0, 2) / (hex + ".bin");
}

bool LocalDiskShaderArtifactStore::Serialize(
	const CookedStageBuild& build,
	std::vector<std::uint8_t>& outBytes,
	std::string& outErrorMessage)
{
	outBytes.clear();
	outBytes.reserve(build.bytecode.size() + 256);
	Files::BinaryBufferWriter writer(outBytes);

	writer.WriteValue(kFormatMagic);
	writer.WriteValue(kFormatVersion);
	writer.WriteValue(static_cast<std::uint16_t>(build.stage));
	writer.WriteValue(static_cast<std::uint8_t>(build.format));
	writer.WriteValue(build.bytecodeHash);
	writer.WriteValue(build.backendVersion);

	if (!writer.WriteStringWithUInt32Length(build.sourcePath, outErrorMessage) ||
		!writer.WriteStringWithUInt32Length(build.entryPoint, outErrorMessage) ||
		!writer.WriteStringWithUInt32Length(build.debugArtifact, outErrorMessage) ||
		!writer.WriteStringWithUInt32Length(build.backendName, outErrorMessage))
	{
		return false;
	}

	if (build.bytecode.size() > static_cast<std::size_t>((std::numeric_limits<std::uint32_t>::max)()))
	{
		outErrorMessage = "Cache serialization failed: bytecode too large";
		return false;
	}

	const std::uint32_t bytecodeSize = static_cast<std::uint32_t>(build.bytecode.size());
	writer.WriteValue(bytecodeSize);
	writer.WriteBytes(std::span<const std::uint8_t>(build.bytecode.data(), build.bytecode.size()));

	if (!SerializeReflection(build.reflection, writer, outErrorMessage))
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
	Files::BinarySpanReader reader(bytes);
	std::uint32_t magic = 0;
	std::uint32_t version = 0;
	std::uint16_t stage = 0;
	std::uint8_t format = 0;
	std::uint64_t bytecodeHash = 0;
	std::uint64_t backendVersion = 0;

	if (!reader.ReadValue(magic, outErrorMessage) || !reader.ReadValue(version, outErrorMessage) ||
		!reader.ReadValue(stage, outErrorMessage) || !reader.ReadValue(format, outErrorMessage) ||
		!reader.ReadValue(bytecodeHash, outErrorMessage) || !reader.ReadValue(backendVersion, outErrorMessage))
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

	if (!reader.ReadStringWithUInt32Length(build.sourcePath, outErrorMessage) || !reader.ReadStringWithUInt32Length(build.entryPoint, outErrorMessage) ||
		!reader.ReadStringWithUInt32Length(build.debugArtifact, outErrorMessage) || !reader.ReadStringWithUInt32Length(build.backendName, outErrorMessage))
	{
		outErrorMessage = "Cache deserialize failed: truncated string payload";
		return false;
	}

	std::uint32_t bytecodeSize = 0;
	std::span<const std::uint8_t> bytecodeBytes;
	if (!reader.ReadValue(bytecodeSize, outErrorMessage) || !reader.ReadBytes(bytecodeSize, bytecodeBytes, outErrorMessage))
	{
		outErrorMessage = "Cache deserialize failed: truncated bytecode payload";
		return false;
	}

	build.bytecode.assign(bytecodeBytes.begin(), bytecodeBytes.end());

	if (!DeserializeReflection(reader, build.reflection, outErrorMessage))
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
    Files::BinaryBufferWriter& writer,
    std::string& outErrorMessage)
{
	writer.WriteValue(reflection.ThreadGroupSize[0]);
	writer.WriteValue(reflection.ThreadGroupSize[1]);
	writer.WriteValue(reflection.ThreadGroupSize[2]);
	writer.WriteValue(reflection.EntryFlags);
	writer.WriteValue(reflection.WaveSize);

	writer.WriteValue(static_cast<std::uint32_t>(reflection.Bindings.size()));
	for (const ShaderReflectionResourceBinding& b : reflection.Bindings)
	{
		if (!writer.WriteStringWithUInt32Length(b.Name, outErrorMessage))
		{
			return false;
		}
		writer.WriteValue(b.Kind);
		writer.WriteValue(b.Dimension);
		writer.WriteValue(static_cast<std::uint8_t>(b.IsReadOnly ? 1 : 0));
		writer.WriteValue(b.Set);
		writer.WriteValue(b.Slot);
		writer.WriteValue(b.ArrayCount);
		writer.WriteValue(b.SizeInBytes);
		writer.WriteValue(b.ConstantBufferIndex);
	}

	writer.WriteValue(static_cast<std::uint32_t>(reflection.ConstantBuffers.size()));
	for (const ShaderReflectionConstantBuffer& cb : reflection.ConstantBuffers)
	{
		if (!writer.WriteStringWithUInt32Length(cb.Name, outErrorMessage))
		{
			return false;
		}
		writer.WriteValue(cb.SizeInBytes);
		writer.WriteValue(static_cast<std::uint32_t>(cb.Members.size()));
		for (const ShaderReflectionConstantBufferMember& m : cb.Members)
		{
			if (!writer.WriteStringWithUInt32Length(m.Name, outErrorMessage))
			{
				return false;
			}
			writer.WriteValue(m.OffsetInBytes);
			writer.WriteValue(m.SizeInBytes);
			writer.WriteValue(m.ArrayCount);
			writer.WriteValue(m.ArrayStrideInBytes);
			writer.WriteValue(m.ScalarType);
			writer.WriteValue(m.RowCount);
			writer.WriteValue(m.ColumnCount);
		}
	}

	writer.WriteValue(static_cast<std::uint32_t>(reflection.InputElements.size()));
	for (const ShaderReflectionInputElement& e : reflection.InputElements)
	{
		if (!writer.WriteStringWithUInt32Length(e.Semantic, outErrorMessage))
		{
			return false;
		}
		writer.WriteValue(e.SemanticIndex);
		writer.WriteValue(e.Location);
		writer.WriteValue(e.ScalarType);
		writer.WriteValue(e.ComponentCount);
	}

	writer.WriteValue(static_cast<std::uint32_t>(reflection.PushConstants.size()));
	for (const ShaderReflectionPushConstantRange& r : reflection.PushConstants)
	{
		writer.WriteValue(r.OffsetInBytes);
		writer.WriteValue(r.SizeInBytes);
		writer.WriteValue(r.VisibilityMask);
	}

	writer.WriteValue(static_cast<std::uint32_t>(reflection.SpecializationConstants.size()));
	for (const ShaderReflectionSpecializationConstant& s : reflection.SpecializationConstants)
	{
		if (!writer.WriteStringWithUInt32Length(s.Name, outErrorMessage))
		{
			return false;
		}
		writer.WriteValue(s.ConstantId);
		writer.WriteValue(s.DefaultValueBits);
		writer.WriteValue(s.ScalarType);
	}

	return true;
}

bool LocalDiskShaderArtifactStore::DeserializeReflection(
	Files::BinarySpanReader& reader,
    ShaderReflection& outReflection,
    std::string& outErrorMessage)
{
	auto fail = [&](const char* msg) {
		outErrorMessage = msg;
		return false;
	};

	if (!reader.ReadValue(outReflection.ThreadGroupSize[0], outErrorMessage) ||
	    !reader.ReadValue(outReflection.ThreadGroupSize[1], outErrorMessage) ||
	    !reader.ReadValue(outReflection.ThreadGroupSize[2], outErrorMessage) || !reader.ReadValue(outReflection.EntryFlags, outErrorMessage) ||
	    !reader.ReadValue(outReflection.WaveSize, outErrorMessage))
	{
		return fail("Cache deserialize failed: truncated reflection header");
	}

	std::uint32_t bindingCount = 0;
	if (!reader.ReadValue(bindingCount, outErrorMessage))
	{
		return fail("Cache deserialize failed: truncated bindings count");
	}
	outReflection.Bindings.resize(bindingCount);
	for (std::uint32_t i = 0; i < bindingCount; ++i)
	{
		ShaderReflectionResourceBinding& b = outReflection.Bindings[i];
		std::uint8_t isReadOnly = 0;
		if (!reader.ReadStringWithUInt32Length(b.Name, outErrorMessage) || !reader.ReadValue(b.Kind, outErrorMessage) || !reader.ReadValue(b.Dimension, outErrorMessage) ||
		    !reader.ReadValue(isReadOnly, outErrorMessage) || !reader.ReadValue(b.Set, outErrorMessage) || !reader.ReadValue(b.Slot, outErrorMessage) ||
		    !reader.ReadValue(b.ArrayCount, outErrorMessage) || !reader.ReadValue(b.SizeInBytes, outErrorMessage) ||
		    !reader.ReadValue(b.ConstantBufferIndex, outErrorMessage))
		{
			return fail("Cache deserialize failed: truncated binding record");
		}
		b.IsReadOnly = isReadOnly != 0;
	}

	std::uint32_t cbCount = 0;
	if (!reader.ReadValue(cbCount, outErrorMessage))
	{
		return fail("Cache deserialize failed: truncated CB count");
	}
	outReflection.ConstantBuffers.resize(cbCount);
	for (std::uint32_t i = 0; i < cbCount; ++i)
	{
		ShaderReflectionConstantBuffer& cb = outReflection.ConstantBuffers[i];
		std::uint32_t memberCount = 0;
		if (!reader.ReadStringWithUInt32Length(cb.Name, outErrorMessage) || !reader.ReadValue(cb.SizeInBytes, outErrorMessage) || !reader.ReadValue(memberCount, outErrorMessage))
		{
			return fail("Cache deserialize failed: truncated CB record");
		}
		cb.Members.resize(memberCount);
		for (std::uint32_t j = 0; j < memberCount; ++j)
		{
			ShaderReflectionConstantBufferMember& m = cb.Members[j];
			if (!reader.ReadStringWithUInt32Length(m.Name, outErrorMessage) || !reader.ReadValue(m.OffsetInBytes, outErrorMessage) ||
			    !reader.ReadValue(m.SizeInBytes, outErrorMessage) || !reader.ReadValue(m.ArrayCount, outErrorMessage) ||
			    !reader.ReadValue(m.ArrayStrideInBytes, outErrorMessage) || !reader.ReadValue(m.ScalarType, outErrorMessage) ||
			    !reader.ReadValue(m.RowCount, outErrorMessage) || !reader.ReadValue(m.ColumnCount, outErrorMessage))
			{
				return fail("Cache deserialize failed: truncated CB member");
			}
		}
	}

	std::uint32_t inputCount = 0;
	if (!reader.ReadValue(inputCount, outErrorMessage))
	{
		return fail("Cache deserialize failed: truncated input count");
	}
	outReflection.InputElements.resize(inputCount);
	for (std::uint32_t i = 0; i < inputCount; ++i)
	{
		ShaderReflectionInputElement& e = outReflection.InputElements[i];
		if (!reader.ReadStringWithUInt32Length(e.Semantic, outErrorMessage) || !reader.ReadValue(e.SemanticIndex, outErrorMessage) || !reader.ReadValue(e.Location, outErrorMessage) ||
		    !reader.ReadValue(e.ScalarType, outErrorMessage) || !reader.ReadValue(e.ComponentCount, outErrorMessage))
		{
			return fail("Cache deserialize failed: truncated input element");
		}
	}

	std::uint32_t pushCount = 0;
	if (!reader.ReadValue(pushCount, outErrorMessage))
	{
		return fail("Cache deserialize failed: truncated push count");
	}
	outReflection.PushConstants.resize(pushCount);
	for (std::uint32_t i = 0; i < pushCount; ++i)
	{
		ShaderReflectionPushConstantRange& r = outReflection.PushConstants[i];
		if (!reader.ReadValue(r.OffsetInBytes, outErrorMessage) || !reader.ReadValue(r.SizeInBytes, outErrorMessage) ||
		    !reader.ReadValue(r.VisibilityMask, outErrorMessage))
		{
			return fail("Cache deserialize failed: truncated push range");
		}
	}

	std::uint32_t specCount = 0;
	if (!reader.ReadValue(specCount, outErrorMessage))
	{
		return fail("Cache deserialize failed: truncated spec count");
	}
	outReflection.SpecializationConstants.resize(specCount);
	for (std::uint32_t i = 0; i < specCount; ++i)
	{
		ShaderReflectionSpecializationConstant& s = outReflection.SpecializationConstants[i];
		if (!reader.ReadStringWithUInt32Length(s.Name, outErrorMessage) || !reader.ReadValue(s.ConstantId, outErrorMessage) ||
		    !reader.ReadValue(s.DefaultValueBits, outErrorMessage) || !reader.ReadValue(s.ScalarType, outErrorMessage))
		{
			return fail("Cache deserialize failed: truncated spec constant");
		}
	}

	return true;
}