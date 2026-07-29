#include "PCH.h"

#include "Cooking/Cache/LocalDiskShaderArtifactStore.h"

#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Diagnostics/Error.h"
#include "ShaderReflection.h"

#include <chrono>

std::filesystem::path LocalDiskShaderArtifactStore::BuildArtifactPath(const ShaderCacheKey& key) const
{
	const std::string hex = key.ToHex();
	return m_rootDirectory / hex.substr(0, 2) / (hex + ".bin");
}

std::vector<std::uint8_t> LocalDiskShaderArtifactStore::Serialize(const CookedStageBuild& build)
{
	std::vector<std::uint8_t> bytes;
	bytes.reserve(build.bytecode.size() + 256);
	Files::BinaryBufferWriter writer(bytes);
	std::string serializationError;

	writer.WriteValue(kFormatMagic);
	writer.WriteValue(kFormatVersion);
	writer.WriteValue(static_cast<std::uint16_t>(build.stage));
	writer.WriteValue(static_cast<std::uint8_t>(build.format));
	writer.WriteValue(build.bytecodeHash);
	writer.WriteValue(build.backendVersion);

	if (!writer.WriteStringWithUInt32Length(build.sourcePath, serializationError) ||
		!writer.WriteStringWithUInt32Length(build.entryPoint, serializationError) ||
		!writer.WriteStringWithUInt32Length(build.debugArtifact, serializationError) ||
		!writer.WriteStringWithUInt32Length(build.backendName, serializationError))
	{
		throw Diagnostics::Error(std::move(serializationError));
	}

	if (build.bytecode.size() > static_cast<std::size_t>((std::numeric_limits<std::uint32_t>::max)()))
	{
		throw Diagnostics::Error("Cache serialization failed: bytecode too large");
	}

	const std::uint32_t bytecodeSize = static_cast<std::uint32_t>(build.bytecode.size());
	writer.WriteValue(bytecodeSize);
	writer.WriteBytes(std::span<const std::uint8_t>(build.bytecode.data(), build.bytecode.size()));

	SerializeReflection(build.reflection, writer);
	return bytes;
}

CookedStageBuild LocalDiskShaderArtifactStore::Deserialize(std::span<const std::uint8_t> bytes)
{
	Files::BinarySpanReader reader(bytes);
	std::string deserializationError;
	std::uint32_t magic = 0;
	std::uint32_t version = 0;
	std::uint16_t stage = 0;
	std::uint8_t format = 0;
	std::uint64_t bytecodeHash = 0;
	std::uint64_t backendVersion = 0;

	if (!reader.ReadValue(magic, deserializationError) || !reader.ReadValue(version, deserializationError) ||
		!reader.ReadValue(stage, deserializationError) || !reader.ReadValue(format, deserializationError) ||
		!reader.ReadValue(bytecodeHash, deserializationError) || !reader.ReadValue(backendVersion, deserializationError))
	{
		throw Diagnostics::Error("Cache deserialize failed: truncated header");
	}

	if (magic != kFormatMagic || version != kFormatVersion)
	{
		throw Diagnostics::Error("Cache deserialize failed: unsupported artifact format");
	}

	CookedStageBuild build;
	build.stage = static_cast<ShaderStage>(stage);
	build.format = static_cast<CookedShaderBinaryFormat>(format);
	build.bytecodeHash = bytecodeHash;
	build.backendVersion = backendVersion;

	if (!reader.ReadStringWithUInt32Length(build.sourcePath, deserializationError) ||
	    !reader.ReadStringWithUInt32Length(build.entryPoint, deserializationError) ||
		!reader.ReadStringWithUInt32Length(build.debugArtifact, deserializationError) ||
	    !reader.ReadStringWithUInt32Length(build.backendName, deserializationError))
	{
		throw Diagnostics::Error("Cache deserialize failed: truncated string payload");
	}

	std::uint32_t bytecodeSize = 0;
	std::span<const std::uint8_t> bytecodeBytes;
	if (!reader.ReadValue(bytecodeSize, deserializationError) ||
	    !reader.ReadBytes(bytecodeSize, bytecodeBytes, deserializationError))
	{
		throw Diagnostics::Error("Cache deserialize failed: truncated bytecode payload");
	}

	build.bytecode.assign(bytecodeBytes.begin(), bytecodeBytes.end());

	DeserializeReflection(reader, build.reflection);
	return build;
}

std::optional<CookedStageBuild> LocalDiskShaderArtifactStore::Find(const ShaderCacheKey& key) const
{
	std::vector<std::uint8_t> bytes;
	const std::filesystem::path artifactPath = BuildArtifactPath(key);
	if (!std::filesystem::exists(artifactPath))
	{
		return std::nullopt;
	}

	std::string fileError;
	if (!Files::TryReadAllBytes(artifactPath, bytes, fileError))
	{
		throw Diagnostics::Error(std::move(fileError));
	}

	return Deserialize(bytes);
}

void LocalDiskShaderArtifactStore::Put(
	const ShaderCacheKey& key,
	const CookedStageBuild& build)
{
	std::string fileError;
	const std::vector<std::uint8_t> bytes = Serialize(build);

	const std::filesystem::path artifactPath = BuildArtifactPath(key);
	const auto nowTicks = std::chrono::steady_clock::now().time_since_epoch().count();
	const std::filesystem::path tempPath = Files::BuildTemporaryPath(artifactPath, "." + std::to_string(nowTicks) + ".tmp");
	if (!Files::TryWriteAllBytes(tempPath, bytes, fileError))
	{
		throw Diagnostics::Error(std::move(fileError));
	}

	if (!Files::TryFinalizeTemporaryFileIfMissing(tempPath, artifactPath, fileError))
	{
		throw Diagnostics::Error(std::move(fileError));
	}
}

void LocalDiskShaderArtifactStore::SerializeReflection(
    const ShaderReflection& reflection,
    Files::BinaryBufferWriter& writer)
{
	std::string serializationError;
	writer.WriteValue(reflection.ThreadGroupSize[0]);
	writer.WriteValue(reflection.ThreadGroupSize[1]);
	writer.WriteValue(reflection.ThreadGroupSize[2]);
	writer.WriteValue(reflection.EntryFlags);
	writer.WriteValue(reflection.WaveSize);

	writer.WriteValue(static_cast<std::uint32_t>(reflection.Bindings.size()));
	for (const ShaderReflectionResourceBinding& b : reflection.Bindings)
	{
		if (!writer.WriteStringWithUInt32Length(b.Name, serializationError))
		{
			throw Diagnostics::Error(std::move(serializationError));
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
		if (!writer.WriteStringWithUInt32Length(cb.Name, serializationError))
		{
			throw Diagnostics::Error(std::move(serializationError));
		}
		writer.WriteValue(cb.SizeInBytes);
		writer.WriteValue(static_cast<std::uint32_t>(cb.Members.size()));
		for (const ShaderReflectionConstantBufferMember& m : cb.Members)
		{
			if (!writer.WriteStringWithUInt32Length(m.Name, serializationError))
			{
				throw Diagnostics::Error(std::move(serializationError));
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
		if (!writer.WriteStringWithUInt32Length(e.Semantic, serializationError))
		{
			throw Diagnostics::Error(std::move(serializationError));
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
		if (!writer.WriteStringWithUInt32Length(s.Name, serializationError))
		{
			throw Diagnostics::Error(std::move(serializationError));
		}
		writer.WriteValue(s.ConstantId);
		writer.WriteValue(s.DefaultValueBits);
		writer.WriteValue(s.ScalarType);
	}

}

void LocalDiskShaderArtifactStore::DeserializeReflection(
	Files::BinarySpanReader& reader,
    ShaderReflection& outReflection)
{
	std::string deserializationError;
	auto reject = [](const char* message) {
		throw Diagnostics::Error(message);
	};

	if (!reader.ReadValue(outReflection.ThreadGroupSize[0], deserializationError) ||
	    !reader.ReadValue(outReflection.ThreadGroupSize[1], deserializationError) ||
	    !reader.ReadValue(outReflection.ThreadGroupSize[2], deserializationError) ||
	    !reader.ReadValue(outReflection.EntryFlags, deserializationError) ||
	    !reader.ReadValue(outReflection.WaveSize, deserializationError))
	{
		reject("Cache deserialize failed: truncated reflection header");
	}

	std::uint32_t bindingCount = 0;
	if (!reader.ReadValue(bindingCount, deserializationError))
	{
		reject("Cache deserialize failed: truncated bindings count");
	}
	outReflection.Bindings.resize(bindingCount);
	for (std::uint32_t i = 0; i < bindingCount; ++i)
	{
		ShaderReflectionResourceBinding& b = outReflection.Bindings[i];
		std::uint8_t isReadOnly = 0;
		if (!reader.ReadStringWithUInt32Length(b.Name, deserializationError) ||
		    !reader.ReadValue(b.Kind, deserializationError) ||
		    !reader.ReadValue(b.Dimension, deserializationError) ||
		    !reader.ReadValue(isReadOnly, deserializationError) ||
		    !reader.ReadValue(b.Set, deserializationError) ||
		    !reader.ReadValue(b.Slot, deserializationError) ||
		    !reader.ReadValue(b.ArrayCount, deserializationError) ||
		    !reader.ReadValue(b.SizeInBytes, deserializationError) ||
		    !reader.ReadValue(b.ConstantBufferIndex, deserializationError))
		{
			reject("Cache deserialize failed: truncated binding record");
		}
		b.IsReadOnly = isReadOnly != 0;
	}

	std::uint32_t cbCount = 0;
	if (!reader.ReadValue(cbCount, deserializationError))
	{
		reject("Cache deserialize failed: truncated CB count");
	}
	outReflection.ConstantBuffers.resize(cbCount);
	for (std::uint32_t i = 0; i < cbCount; ++i)
	{
		ShaderReflectionConstantBuffer& cb = outReflection.ConstantBuffers[i];
		std::uint32_t memberCount = 0;
		if (!reader.ReadStringWithUInt32Length(cb.Name, deserializationError) ||
		    !reader.ReadValue(cb.SizeInBytes, deserializationError) ||
		    !reader.ReadValue(memberCount, deserializationError))
		{
			reject("Cache deserialize failed: truncated CB record");
		}
		cb.Members.resize(memberCount);
		for (std::uint32_t j = 0; j < memberCount; ++j)
		{
			ShaderReflectionConstantBufferMember& m = cb.Members[j];
			if (!reader.ReadStringWithUInt32Length(m.Name, deserializationError) ||
			    !reader.ReadValue(m.OffsetInBytes, deserializationError) ||
			    !reader.ReadValue(m.SizeInBytes, deserializationError) ||
			    !reader.ReadValue(m.ArrayCount, deserializationError) ||
			    !reader.ReadValue(m.ArrayStrideInBytes, deserializationError) ||
			    !reader.ReadValue(m.ScalarType, deserializationError) ||
			    !reader.ReadValue(m.RowCount, deserializationError) ||
			    !reader.ReadValue(m.ColumnCount, deserializationError))
			{
				reject("Cache deserialize failed: truncated CB member");
			}
		}
	}

	std::uint32_t inputCount = 0;
	if (!reader.ReadValue(inputCount, deserializationError))
	{
		reject("Cache deserialize failed: truncated input count");
	}
	outReflection.InputElements.resize(inputCount);
	for (std::uint32_t i = 0; i < inputCount; ++i)
	{
		ShaderReflectionInputElement& e = outReflection.InputElements[i];
		if (!reader.ReadStringWithUInt32Length(e.Semantic, deserializationError) ||
		    !reader.ReadValue(e.SemanticIndex, deserializationError) ||
		    !reader.ReadValue(e.Location, deserializationError) ||
		    !reader.ReadValue(e.ScalarType, deserializationError) ||
		    !reader.ReadValue(e.ComponentCount, deserializationError))
		{
			reject("Cache deserialize failed: truncated input element");
		}
	}

	std::uint32_t pushCount = 0;
	if (!reader.ReadValue(pushCount, deserializationError))
	{
		reject("Cache deserialize failed: truncated push count");
	}
	outReflection.PushConstants.resize(pushCount);
	for (std::uint32_t i = 0; i < pushCount; ++i)
	{
		ShaderReflectionPushConstantRange& r = outReflection.PushConstants[i];
		if (!reader.ReadValue(r.OffsetInBytes, deserializationError) ||
		    !reader.ReadValue(r.SizeInBytes, deserializationError) ||
		    !reader.ReadValue(r.VisibilityMask, deserializationError))
		{
			reject("Cache deserialize failed: truncated push range");
		}
	}

	std::uint32_t specCount = 0;
	if (!reader.ReadValue(specCount, deserializationError))
	{
		reject("Cache deserialize failed: truncated spec count");
	}
	outReflection.SpecializationConstants.resize(specCount);
	for (std::uint32_t i = 0; i < specCount; ++i)
	{
		ShaderReflectionSpecializationConstant& s = outReflection.SpecializationConstants[i];
		if (!reader.ReadStringWithUInt32Length(s.Name, deserializationError) ||
		    !reader.ReadValue(s.ConstantId, deserializationError) ||
		    !reader.ReadValue(s.DefaultValueBits, deserializationError) ||
		    !reader.ReadValue(s.ScalarType, deserializationError))
		{
			reject("Cache deserialize failed: truncated spec constant");
		}
	}
}
