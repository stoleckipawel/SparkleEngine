#include "PCH.h"

#include "ShaderRecook/ShaderRecookPublicationReader.h"

#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Json/JsonReader.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

ShaderRecookPublicationReadResult ShaderRecookPublicationReader::Read(const std::filesystem::path& publicationPath) noexcept
{
	ShaderRecookPublicationReadResult result;
	std::error_code errorCode;
	if (!std::filesystem::exists(publicationPath, errorCode) || errorCode)
	{
		result.Missing = true;
		return result;
	}

	std::vector<std::uint8_t> bytes;
	std::string readErrorMessage;
	if (!Files::TryReadAllBytes(publicationPath, bytes, readErrorMessage))
	{
		result.Diagnostic =
		    "Shader recook publication could not be read; reload rejected before touching active shaders. " + readErrorMessage;
		return result;
	}

	const std::string text(bytes.begin(), bytes.end());
	if (text.empty())
	{
		result.Diagnostic = "Shader recook publication is empty or partially written; reload rejected before touching active shaders.";
		return result;
	}

	result = Parse(text);
	if (!result.Publication.has_value())
	{
		return result;
	}

	const ShaderRecookPublication& publication = *result.Publication;
	if (publication.GlobalShaderMapPath.lexically_normal() != Filesystem::GetGlobalShaderMapPath().lexically_normal()
	    || publication.CookedShaderLibraryPath.lexically_normal() != Filesystem::GetCookedShaderLibraryPath().lexically_normal())
	{
		result.Publication.reset();
		result.Diagnostic = "Shader recook publication names artifacts outside the active cooked shader authority; reload rejected.";
		return result;
	}

	std::vector<std::uint8_t> mapBytes;
	std::vector<std::uint8_t> libraryBytes;
	if (!Files::TryReadAllBytes(publication.GlobalShaderMapPath, mapBytes, readErrorMessage)
	    || !Files::TryReadAllBytes(publication.CookedShaderLibraryPath, libraryBytes, readErrorMessage))
	{
		result.Publication.reset();
		result.Diagnostic = "Shader recook artifacts could not be read; reload rejected. " + readErrorMessage;
		return result;
	}
	if (Hash::Fnv1a64(mapBytes.data(), mapBytes.size()) != publication.GlobalShaderMapHash
	    || Hash::Fnv1a64(libraryBytes.data(), libraryBytes.size()) != publication.CookedShaderLibraryHash)
	{
		result.Publication.reset();
		result.Diagnostic = "Shader recook artifact hashes do not match the published generation; reload rejected.";
	}
	return result;
}

ShaderRecookPublicationReadResult ShaderRecookPublicationReader::Parse(std::string_view text) noexcept
{
	ShaderRecookPublicationReadResult result;
	std::string schema;
	std::string status;
	std::string globalShaderMap;
	std::string globalShaderMapHashText;
	std::string cookedShaderLibrary;
	std::string cookedShaderLibraryHashText;
	std::uint64_t publicationId = 0;
	std::uint64_t publishedAtUnixMs = 0;
	std::uint64_t globalShaderMapHash = 0;
	std::uint64_t cookedShaderLibraryHash = 0;
	if (!Json::TryReadStringProperty(text, "schema", schema) || !Json::TryReadStringProperty(text, "status", status)
	    || !Json::TryReadUInt64Property(text, "publicationId", publicationId)
	    || !Json::TryReadUInt64Property(text, "publishedAtUnixMs", publishedAtUnixMs)
	    || !Json::TryReadStringProperty(text, "globalShaderMap", globalShaderMap)
	    || !Json::TryReadStringProperty(text, "globalShaderMapHash", globalShaderMapHashText)
	    || !Json::TryReadStringProperty(text, "cookedShaderLibrary", cookedShaderLibrary)
	    || !Json::TryReadStringProperty(text, "cookedShaderLibraryHash", cookedShaderLibraryHashText)
	    || !Json::TryParseHexUInt64(globalShaderMapHashText, globalShaderMapHash)
	    || !Json::TryParseHexUInt64(cookedShaderLibraryHashText, cookedShaderLibraryHash))
	{
		result.Diagnostic = "Shader recook publication is invalid or partially written; reload rejected before touching active shaders.";
		return result;
	}

	if (schema != "sparkle.shaderRecookResult")
	{
		result.Diagnostic = "Shader recook publication has unsupported schema '" + schema + "'; reload rejected.";
		return result;
	}

	if (status != "succeeded")
	{
		result.Diagnostic =
		    "Shader recook publication status is '" + status + "', not 'succeeded'; reload rejected before touching active shaders.";
		return result;
	}

	result.Publication = ShaderRecookPublication{
	    .PublicationId = publicationId,
	    .PublishedAtUnixMs = publishedAtUnixMs,
	    .GlobalShaderMapHash = globalShaderMapHash,
	    .CookedShaderLibraryHash = cookedShaderLibraryHash,
	    .GlobalShaderMapPath = std::filesystem::path(globalShaderMap),
	    .CookedShaderLibraryPath = std::filesystem::path(cookedShaderLibrary),
	    .Status = std::move(status)};
	return result;
}
