---
description: "Use when: implementing phase 1 of Sparkle shader layout automation: metadata descriptors and UE-style shader parameter macros."
name: "Sparkle Shader Layout Automation Phase 1 Metadata"
argument-hint: "Implement metadata descriptors and macros only"
agent: "agent"
---

# Phase 1 — Metadata Foundation and UE-style Macros

Implement only the metadata and macro foundation for Sparkle shader layout automation. Do not migrate built-in shaders or delete manual layout helpers in this phase unless required to compile small compatibility tests.

## Goal

Make `BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )` capable of describing both:

1. HLSL-reflected shader resources.
2. Engine layout-only resources such as render/depth targets.

This phase prepares the data needed for later generated `PassParameterLayout` creation.

## Files to inspect first

- `Engine/RHI/Public/Shaders/Authoring/ShaderParameterStruct.h`
- `Engine/RHI/Public/Shaders/Authoring/GlobalShader.h`
- `Engine/RHI/Private/Shaders/ShaderAuthoring.cpp`
- `Engine/RHI/Public/ShaderParameters/PassParameterLayout.h`
- `Tools/ShaderCompiler/Private/Verification/ShaderParameterStructVerifier.cpp`
- `Engine/RHI/Private/Shaders/ForwardOpaqueShaders.cpp`
- `Engine/RHI/Private/Shaders/ShadowOpaqueShaders.cpp`
- `Engine/RHI/Private/Shaders/ComputeClearShader.cpp`

## Required design

Extend `ShaderParameterStructFieldDescriptor` or equivalent descriptor data so each field can carry these concepts:

```cpp
struct ShaderParameterStructFieldDescriptor
{
    std::string LayoutName;        // engine logical binding name, e.g. PerFrame
    std::string ShaderName;        // reflected HLSL symbol, e.g. PerFrameConstantBufferData
    ShaderParameterSemanticKind SemanticKind;
    ShaderParameterResourceDomain ResourceDomain;
    ShaderParameterAccess Access;
    ShaderStageVisibility Visibility;
    std::uint32_t ArrayCount;
    std::uint32_t ValueSizeInBytes;
    std::uint32_t ValueAlignmentInBytes;
    bool Reflected;
};
```

Use names/enums that fit the existing codebase. Do not invent extra namespace nesting just to group these types.

## Required macro vocabulary

Add explicit UE-style macros or equivalent helpers for these categories:

### CBuffer / uniform data alias

Purpose: map a logical layout binding name to a reflected HLSL cbuffer symbol and the real C++ payload type.

Examples:

```cpp
SHADER_PARAMETER_CBUFFER(PerFrame, PerFrameConstantBufferData)
SHADER_PARAMETER_CBUFFER(PerObjectPS, PerObjectPSConstantBufferData)
```

If existing HLSL symbol naming requires different layout and shader names, support an explicit form:

```cpp
SHADER_PARAMETER_CBUFFER_NAMED(PerFrame, PerFrameConstantBufferData, PerFrameConstantBufferData)
```

### Textures / SRVs

Purpose: create individual texture bindings; do not create `MaterialTextures[5]` as the new model.

Examples:

```cpp
SHADER_PARAMETER_TEXTURE(Texture2D, TextureBaseColor)
SHADER_PARAMETER_TEXTURE(Texture2D, TextureNormal)
SHADER_PARAMETER_TEXTURE(Texture2D, ShadowMap0)
```

Support array count when needed for non-material arrays.

### UAVs / writable resources

Purpose: support compute shaders such as `ComputeClear`.

Example:

```cpp
SHADER_PARAMETER_UAV(RWTexture2D, Output)
```

If HLSL currently names it `OutputTexture` but runtime layout wants `Output`, provide an alias form.

### Samplers

Purpose: support both shared/global and unique/reflected sampler policies.

Examples:

```cpp
SHADER_PARAMETER_SHARED_SAMPLER(SamplerAniso16xWrap)
SHADER_PARAMETER_UNIQUE_SAMPLER(SamplerState, MySampler)
```

The shared sampler form may be `Reflected == true` if the HLSL has a matching sampler symbol, or may use explicit policy metadata if the existing shader model groups shared samplers through a table. Choose the smallest compatible step, but keep metadata expressive enough for both policies.

### Pass IO

Purpose: represent render/depth targets in pass/package ABI while excluding them from shader reflection verification.

Examples:

```cpp
SHADER_PARAMETER_RENDER_TARGET(BackBuffer)
SHADER_PARAMETER_DEPTH_TARGET(MainDepth)
```

These fields must set `Reflected = false`.

## Compatibility

- Existing `SHADER_PARAMETER`, `SHADER_PARAMETER_TEXTURE`, and `SHADER_PARAMETER_SAMPLER` forms may remain temporarily as wrappers.
- Do not rely on naming conventions as the long-term path.
- Keep wrapper behavior conservative and easy to delete in a later phase.

## Acceptance criteria

- Existing shader declarations still compile.
- Descriptor metadata can represent:
  - logical vs reflected names,
  - layout-only pass IO,
  - C++ uniform value size,
  - texture arrays,
  - shared and unique sampler policy.
- No manual `PassParameterLayout` code is added.
- No runtime behavior changes are required in this phase.

## Validation

- Run focused compilation/build checks for the RHI and ShaderCompiler targets if available.
- If a full build is needed, prefer the workspace CMake build integration. Use repository scripts only for project-specific cooking/build flows that CMake Tools does not cover.
- Fix only errors caused by this phase.
