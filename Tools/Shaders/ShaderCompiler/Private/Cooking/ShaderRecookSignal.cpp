#include "PCH.h"

#include "Cooking/ShaderRecookSignal.h"

#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Json/JsonWriter.h"
#include "Core/Public/Paths/DirectoryPaths.h"

#include <chrono>

bool ShaderRecookSignal::Write(
    const std::filesystem::path& registryReadPath,
    const std::filesystem::path& publishedRegistryPath,
    const std::filesystem::path& signalStoragePath,
    const std::filesystem::path& publishedSignalPath,
    ShaderRecookSignalResult& outResult,
    std::string& outErrorMessage)
{
	outResult = {};
	outResult.signalPath = publishedSignalPath;

	std::vector<std::uint8_t> registryBytes;
	if (!Files::TryReadAllBytes(registryReadPath, registryBytes, outErrorMessage))
	{
		outErrorMessage = "Failed to read shader registry for recook publication - " + outErrorMessage;
		return false;
	}

	outResult.registryHash = Hash::Fnv1a64(registryBytes.data(), registryBytes.size());
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
	writer.WriteHexUInt64("registryHash", outResult.registryHash);
	const std::string contents = writer.Finish();

	if (!Files::TryWriteAllTextAtomic(signalStoragePath, contents, outErrorMessage))
	{
		return false;
	}

	outErrorMessage.clear();
	return true;
}
