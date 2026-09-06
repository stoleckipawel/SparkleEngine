#include "PCH.h"

#include "Cooking/ShaderRecookSignal.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Json/JsonWriter.h"

#include <chrono>

void ShaderRecookSignal::Write(
    const std::filesystem::path& mapReadPath,
    const std::filesystem::path& publishedMapPath,
    const std::filesystem::path& libraryReadPath,
    const std::filesystem::path& publishedLibraryPath,
    const std::filesystem::path& signalStoragePath)
{
	std::vector<std::uint8_t> mapBytes;
	std::vector<std::uint8_t> libraryBytes;
	std::string fileError;
	if (!Files::TryReadAllBytes(mapReadPath, mapBytes, fileError) || !Files::TryReadAllBytes(libraryReadPath, libraryBytes, fileError))
	{
		throw Diagnostics::Error("Failed to read cooked shader artifacts for recook publication - " + fileError);
	}

	const std::uint64_t mapHash = Hash::Fnv1a64(mapBytes.data(), mapBytes.size());
	const std::uint64_t libraryHash = Hash::Fnv1a64(libraryBytes.data(), libraryBytes.size());
	const auto now = std::chrono::system_clock::now().time_since_epoch();
	const auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
	const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
	Json::ObjectWriter writer;
	writer.WriteString("schema", "sparkle.shaderRecookResult");
	writer.WriteString("status", "succeeded");
	writer.WriteUInt64("publicationId", static_cast<std::uint64_t>(nanoseconds));
	writer.WriteUInt64("publishedAtUnixMs", static_cast<std::uint64_t>(milliseconds));
	writer.WriteString("globalShaderMap", publishedMapPath.generic_string());
	writer.WriteHexUInt64("globalShaderMapHash", mapHash);
	writer.WriteString("cookedShaderLibrary", publishedLibraryPath.generic_string());
	writer.WriteHexUInt64("cookedShaderLibraryHash", libraryHash);
	const std::string contents = writer.Finish();

	if (!Files::TryWriteAllTextAtomic(signalStoragePath, contents, fileError))
	{
		throw Diagnostics::Error(fileError);
	}
}
