#include "PCH.h"

#include "Fbx/FbxEmbeddedTextureImporter.h"

#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Core/Public/Strings/StringUtils.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <initializer_list>
#include <limits>

class FbxEmbeddedTextureEncoding final
{
public:
	struct Payload final
	{
		std::vector<std::uint8_t> Bytes;
		std::string Extension;
	};

	static Payload BuildFilePayload(const aiTexture& texture)
	{
		if (texture.pcData == nullptr || texture.mWidth == 0)
		{
			throw Diagnostics::Error("Embedded FBX texture has no pixel payload.");
		}

		Payload payload;
		if (texture.mHeight == 0)
		{
			payload.Bytes.assign(
			    reinterpret_cast<const std::uint8_t*>(texture.pcData),
			    reinterpret_cast<const std::uint8_t*>(texture.pcData) + texture.mWidth);
			payload.Extension = ResolveCompressedExtension(texture, payload.Bytes);
			if (payload.Extension.empty())
			{
				throw Diagnostics::Error("Embedded FBX texture uses an unsupported or unidentified compressed format.");
			}
			return payload;
		}

		if (texture.mWidth > (std::numeric_limits<std::uint16_t>::max)() || texture.mHeight > (std::numeric_limits<std::uint16_t>::max)()
		    || texture.mWidth > (std::numeric_limits<std::size_t>::max)() / texture.mHeight)
		{
			throw Diagnostics::Error("Embedded FBX texture dimensions exceed the import cache format.");
		}

		const std::size_t pixelCount = static_cast<std::size_t>(texture.mWidth) * static_cast<std::size_t>(texture.mHeight);
		if (pixelCount > ((std::numeric_limits<std::size_t>::max)() - 18u) / sizeof(aiTexel))
		{
			throw Diagnostics::Error("Embedded FBX texture payload size overflows.");
		}

		static_assert(sizeof(aiTexel) == 4);
		payload.Bytes.assign(18u + pixelCount * sizeof(aiTexel), 0u);
		payload.Bytes[2] = 2u;
		WriteUInt16(payload.Bytes, 12u, static_cast<std::uint16_t>(texture.mWidth));
		WriteUInt16(payload.Bytes, 14u, static_cast<std::uint16_t>(texture.mHeight));
		payload.Bytes[16] = 32u;
		payload.Bytes[17] = 0x28u;
		std::memcpy(payload.Bytes.data() + 18u, texture.pcData, pixelCount * sizeof(aiTexel));
		payload.Extension = ".tga";
		return payload;
	}

private:
	static void WriteUInt16(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint16_t value) noexcept
	{
		bytes[offset] = static_cast<std::uint8_t>(value & 0xFFu);
		bytes[offset + 1u] = static_cast<std::uint8_t>((value >> 8u) & 0xFFu);
	}

	static std::string NormalizeExtension(std::string extension)
	{
		if (extension.empty())
		{
			return {};
		}
		extension = Strings::ToLowerCopy(extension);
		if (extension.front() != '.')
		{
			extension.insert(extension.begin(), '.');
		}
		if (extension.size() > 17u || !std::all_of(extension.begin() + 1, extension.end(), IsExtensionCharacter))
		{
			return {};
		}
		if (extension == ".jpeg")
		{
			return ".jpg";
		}
		return extension;
	}

	static std::string ResolveCompressedExtension(const aiTexture& texture, const std::vector<std::uint8_t>& bytes)
	{
		if (const std::string hint = NormalizeExtension(texture.achFormatHint); !hint.empty())
		{
			return hint;
		}
		if (const std::string fileExtension = NormalizeExtension(
		        texture.mFilename.length > 0 ? std::filesystem::path(texture.mFilename.C_Str()).extension().string() : std::string{});
		    !fileExtension.empty())
		{
			return fileExtension;
		}

		if (StartsWith(bytes, {0x89u, 0x50u, 0x4Eu, 0x47u, 0x0Du, 0x0Au, 0x1Au, 0x0Au}))
		{
			return ".png";
		}
		if (StartsWith(bytes, {0xFFu, 0xD8u, 0xFFu}))
		{
			return ".jpg";
		}
		if (StartsWith(bytes, {0x44u, 0x44u, 0x53u, 0x20u}))
		{
			return ".dds";
		}
		if (StartsWith(bytes, {0x42u, 0x4Du}))
		{
			return ".bmp";
		}
		if (StartsWith(bytes, {0x47u, 0x49u, 0x46u, 0x38u}))
		{
			return ".gif";
		}
		if (StartsWith(bytes, {0x76u, 0x2Fu, 0x31u, 0x01u}))
		{
			return ".exr";
		}
		if (StartsWith(bytes, {0x23u, 0x3Fu, 0x52u, 0x41u, 0x44u, 0x49u, 0x41u, 0x4Eu, 0x43u, 0x45u})
		    || StartsWith(bytes, {0x23u, 0x3Fu, 0x52u, 0x47u, 0x42u, 0x45u}))
		{
			return ".hdr";
		}
		return {};
	}

	static bool IsExtensionCharacter(char character) noexcept
	{
		return (character >= 'a' && character <= 'z') || (character >= '0' && character <= '9');
	}

	static bool StartsWith(const std::vector<std::uint8_t>& bytes, std::initializer_list<std::uint8_t> signature) noexcept
	{
		return bytes.size() >= signature.size() && std::equal(signature.begin(), signature.end(), bytes.begin());
	}
};

std::vector<std::filesystem::path> FbxEmbeddedTextureImporter::ExtractTextures(const aiScene& scene)
{
	std::vector<std::filesystem::path> texturePaths(scene.mNumTextures);
	for (unsigned int textureIndex = 0; textureIndex < scene.mNumTextures; ++textureIndex)
	{
		const aiTexture* texture = scene.mTextures[textureIndex];
		if (texture == nullptr)
		{
			throw Diagnostics::Error("FBX embedded texture " + std::to_string(textureIndex) + " is null.");
		}

		const FbxEmbeddedTextureEncoding::Payload payload = FbxEmbeddedTextureEncoding::BuildFilePayload(*texture);

		const std::uint64_t contentHash = Hash::Fnv1a64(payload.Bytes.data(), payload.Bytes.size());
		const std::filesystem::path cachePath =
		    Paths::ImportedTextureCacheRoot() / (Formatting::FormatHexUInt64(contentHash) + payload.Extension);
		std::error_code errorCode;
		const bool cacheEntryExists = std::filesystem::exists(cachePath, errorCode);
		if (errorCode)
		{
			throw Diagnostics::Error("Failed to inspect embedded texture cache entry '" + cachePath.string() + "': " + errorCode.message());
		}

		std::string fileError;
		if (cacheEntryExists)
		{
			std::uint64_t cachedHash = 0;
			if (!Hash::TryFnv1a64File(cachePath, cachedHash, fileError))
			{
				throw Diagnostics::Error(fileError);
			}
			if (cachedHash != contentHash)
			{
				throw Diagnostics::Error("Embedded texture cache entry is corrupt: '" + cachePath.string() + "'.");
			}
		}
		else
		{
			const auto nowTicks = std::chrono::steady_clock::now().time_since_epoch().count();
			const std::filesystem::path temporaryPath = Files::BuildTemporaryPath(cachePath, "." + std::to_string(nowTicks) + ".tmp");
			if (!Files::TryWriteAllBytes(temporaryPath, payload.Bytes, fileError))
			{
				Files::CleanupTemporaryFile(temporaryPath);
				throw Diagnostics::Error(fileError);
			}
			if (!Files::TryFinalizeTemporaryFileIfMissing(temporaryPath, cachePath, fileError))
			{
				Files::CleanupTemporaryFile(temporaryPath);
				throw Diagnostics::Error(fileError);
			}

			std::uint64_t finalizedHash = 0;
			if (!Hash::TryFnv1a64File(cachePath, finalizedHash, fileError))
			{
				throw Diagnostics::Error(fileError);
			}
			if (finalizedHash != contentHash)
			{
				throw Diagnostics::Error(
				    "Embedded texture cache entry does not match its content-addressed name: '" + cachePath.string() + "'.");
			}
		}

		texturePaths[textureIndex] = Paths::Normalize(cachePath);
	}

	return texturePaths;
}
