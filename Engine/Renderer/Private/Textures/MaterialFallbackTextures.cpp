#include "PCH.h"
#include "Renderer/Public/Textures/MaterialFallbackTextures.h"

namespace
{
	constexpr MaterialFallbackTextureDesc kFallbackTextureDescs[] = {
	    {"Albedo", DefaultTexture::White},
	    {"Normal", DefaultTexture::FlatNormal},
	    {"MetallicRoughness", DefaultTexture::DefaultMetallicRoughness},
	    {"Occlusion", DefaultTexture::White},
	    {"Emissive", DefaultTexture::Black}};

	constexpr MaterialFallbackTextureDesc kUnknownFallbackDesc{};
}  // namespace

const MaterialFallbackTextureDesc& MaterialFallbackTextures::GetDesc(MaterialFallbackTexture type)
{
	const auto index = static_cast<std::size_t>(type);
	return index < static_cast<std::size_t>(MaterialFallbackTexture::Count) ? kFallbackTextureDescs[index] : kUnknownFallbackDesc;
}

const char* MaterialFallbackTextures::GetName(MaterialFallbackTexture type)
{
	return GetDesc(type).name;
}

DefaultTexture MaterialFallbackTextures::GetDefaultTexture(MaterialFallbackTexture type)
{
	return GetDesc(type).defaultTexture;
}