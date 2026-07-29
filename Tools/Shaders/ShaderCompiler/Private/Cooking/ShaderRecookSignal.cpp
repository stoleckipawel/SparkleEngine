#include "PCH.h"

#include "Cooking/ShaderRecookSignal.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Json/JsonWriter.h"
#include "Core/Public/Paths/DirectoryPaths.h"

#include <chrono>

void ShaderRecookSignal::Write(
    const std::filesystem::path& registryReadPath,
    const std::filesystem::path& publishedRegistryPath,
    const std::filesystem::path& signalStoragePath)
{
	std::vector<std::uint8_t> registryBytes;
	std::string fileError;
	if (!Files::TryReadAllBytes(registryReadPath, registryBytes, fileError))
	{
		throw Diagnostics::Error("Failed to read shader registry for recook publication - " + fileError);
	}

	const std::uint64_t registryHash = Hash::Fnv1a64(registryBytes.data(), registryBytes.size());
	const auto now = std::chrono::system_clock::now().time_since_epoch();
	const auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
	const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
	Json::ObjectWriter writer;
	writer.WriteString("schema", "sparkle.shaderRecookResult");
	writer.WriteUInt64("schemaVersion", 1);
	writer.WriteString("status", "succeeded");
	writer.WriteUInt64("publicationId", static_cast<std::uint64_t>(nanoseconds));
	writer.WriteUInt64("publishedAtUnixMs", static_cast<std::uint64_t>(milliseconds));
	writer.WriteString("registry", publishedRegistryPath.generic_string());
	writer.WriteHexUInt64("registryHash", registryHash);
	const std::string contents = writer.Finish();

	if (!Files::TryWriteAllTextAtomic(signalStoragePath, contents, fileError))
	{
		throw Diagnostics::Error(std::move(fileError));
	}
}
