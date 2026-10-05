#include "PCH.h"
#include "Passes/Lighting/Direct/DirectLightingControls.h"

#include "Passes/Lighting/Shadows/DirectShadowControls.h"
#include "Core/Public/Console/CVar.h"
#include "Core/Public/Hash/HashUtils.h"
#include <format>

static ConsoleVariable<bool> CVarDirectDiffuse("r.Lighting.Direct.Diffuse", true, "Evaluate direct diffuse lighting.");
static ConsoleVariable<bool> CVarDirectSpecular("r.Lighting.Direct.Specular", true, "Evaluate direct specular lighting.");
static ConsoleVariable<bool> CVarDirectSubsurface("r.Lighting.Direct.Subsurface", true, "Evaluate direct subsurface lighting.");

bool IsDirectDiffuseEnabled() noexcept
{
	return CVarDirectDiffuse.Get();
}

bool IsDirectSpecularEnabled() noexcept
{
	return CVarDirectSpecular.Get();
}

bool IsDirectLightingAdmitted() noexcept
{
	return IsDirectDiffuseEnabled() || IsDirectSpecularEnabled() || IsDirectSubsurfaceEnabled();
}

bool IsDirectSubsurfaceEnabled() noexcept
{
	return CVarDirectSubsurface.Get();
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
	if (!IsDirectDiffuseEnabled())
	{
		return false;
	}
	RequireDirectLightingProduct(input, output, "diffuse");
	return true;
}

bool IsDirectSpecularActive(FrameGraphTextureHandle input, FrameGraphTextureHandle output) noexcept
{
	if (!IsDirectSpecularEnabled())
	{
		return false;
	}
	RequireDirectLightingProduct(input, output, "specular");
	return true;
}

bool IsDirectSubsurfaceActive(FrameGraphTextureHandle input, FrameGraphTextureHandle output) noexcept
{
	if (!IsDirectSubsurfaceEnabled())
	{
		return false;
	}
	RequireDirectLightingProduct(input, output, "subsurface");
	return true;
}

DirectLightingUniformData BuildDirectLightingUniform(
    FrameGraphTextureHandle baseColorInput,
    FrameGraphTextureHandle materialInput,
    FrameGraphTextureHandle subsurfaceInput,
    FrameGraphTextureHandle diffuseOutput,
    FrameGraphTextureHandle specularOutput,
    FrameGraphTextureHandle subsurfaceOutput) noexcept
{
	return DirectLightingUniformData{
	    .EvaluateDiffuse = IsDirectDiffuseActive(baseColorInput, diffuseOutput) ? 1u : 0u,
	    .EvaluateSpecular = IsDirectSpecularActive(materialInput, specularOutput) ? 1u : 0u,
	    .EvaluateSubsurface = IsDirectSubsurfaceActive(subsurfaceInput, subsurfaceOutput) ? 1u : 0u,
	    .EvaluateShadows = IsDirectShadowsActive() ? 1u : 0u};
}

std::uint64_t AppendDirectLightingHistoryInvalidationHash(std::uint64_t hash) noexcept
{
	hash = Hash::ContinueFnv1a64Value(hash, IsDirectDiffuseEnabled());
	hash = Hash::ContinueFnv1a64Value(hash, IsDirectSpecularEnabled());
	return Hash::ContinueFnv1a64Value(hash, IsDirectSubsurfaceEnabled());
}
