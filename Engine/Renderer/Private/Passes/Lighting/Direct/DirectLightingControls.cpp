#include "PCH.h"
#include "Passes/Lighting/Direct/DirectLightingControls.h"

#include "Core/Public/Console/CVar.h"
#include "Core/Public/Hash/HashUtils.h"
#include <format>

static ConsoleVariable<bool> CVarDirectDiffuse("r.Lighting.Direct.Diffuse", true, "Evaluate direct diffuse lighting.");
static ConsoleVariable<bool> CVarDirectSpecular("r.Lighting.Direct.Specular", true, "Evaluate direct specular lighting.");
static ConsoleVariable<bool> CVarDirectSubsurface("r.Lighting.Direct.Subsurface", true, "Evaluate direct subsurface lighting.");

bool IsDirectLightingAdmitted() noexcept
{
	return CVarDirectDiffuse.Get() || CVarDirectSpecular.Get() || CVarDirectSubsurface.Get();
}

static void RequireDirectLightingProduct(FrameGraphTextureHandle input, FrameGraphTextureHandle output, const char* lobe) noexcept
{
	if (!input.IsValid() || !output.IsValid())
	{
		SPARKLE_DEFINE_LOG_CATEGORY_STATIC(LogDirectLighting, "Renderer.DirectLighting");
		Diagnostics::Fatal(
		    LogDirectLighting,
		    __FILE__,
		    __LINE__,
		    std::format("Enabled direct {} requires its GBuffer input and lighting output.", lobe));
	}
}

bool IsDirectDiffuseActive(FrameGraphTextureHandle input, FrameGraphTextureHandle output) noexcept
{
	if (!CVarDirectDiffuse.Get())
	{
		return false;
	}
	RequireDirectLightingProduct(input, output, "diffuse");
	return true;
}

bool IsDirectSpecularActive(FrameGraphTextureHandle input, FrameGraphTextureHandle output) noexcept
{
	if (!CVarDirectSpecular.Get())
	{
		return false;
	}
	RequireDirectLightingProduct(input, output, "specular");
	return true;
}

bool IsDirectSubsurfaceActive(FrameGraphTextureHandle input, FrameGraphTextureHandle output) noexcept
{
	if (!CVarDirectSubsurface.Get())
	{
		return false;
	}
	RequireDirectLightingProduct(input, output, "subsurface");
	return true;
}

std::uint64_t AppendDirectLightingHistoryInvalidationHash(std::uint64_t hash) noexcept
{
	hash = Hash::ContinueFnv1a64Value(hash, CVarDirectDiffuse.Get());
	hash = Hash::ContinueFnv1a64Value(hash, CVarDirectSpecular.Get());
	return Hash::ContinueFnv1a64Value(hash, CVarDirectSubsurface.Get());
}
