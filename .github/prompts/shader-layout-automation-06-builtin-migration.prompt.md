---
description: "Use when: implementing phase 6 of Sparkle shader layout automation: migrate built-in shaders and delete manual layout helpers."
name: "Sparkle Shader Layout Automation Phase 6 Builtin Migration"
argument-hint: "Migrate ForwardOpaque ShadowOpaque ComputeClear and remove manual helpers"
agent: "agent"
---

# Phase 6 — Built-in Shader Migration and Manual Helper Deletion

Migrate built-in shader packages to generated layout metadata and delete the temporary manual helper path.

## Goal

After this phase, these should be gone:

- `Engine/RHI/Public/Shaders/BuiltinShaderPackageLayouts.h`
- `Engine/RHI/Private/Shaders/BuiltinShaderPackageLayouts.cpp`
- `BuildForwardOpaqueShaderPackageBindingLayout()`
- `BuildShadowOpaqueShaderPackageBindingLayout()`
- `BuildComputeClearShaderPackageBindingLayout()`
- per-shader `BuildPackageBindingLayout()` overrides that only forward to manual helpers

## Files to inspect first

- `Engine/RHI/Private/Shaders/ForwardOpaqueShaders.cpp`
- `Engine/RHI/Private/Shaders/ShadowOpaqueShaders.cpp`
- `Engine/RHI/Private/Shaders/ComputeClearShader.cpp`
- `Engine/RHI/Public/Shaders/BuiltinShaderPackageLayouts.h`
- `Engine/RHI/Private/Shaders/BuiltinShaderPackageLayouts.cpp`
- `Engine/Renderer/Private/Pipeline/RenderPassPipelineTraits.h`
- `Engine/Renderer/Private/Passes/ForwardOpaquePass.cpp`
- `Tools/ShaderCompiler/Private/Cooking/ShaderCookPlanner.cpp`

## ForwardOpaque target metadata

Target logical bindings:

```text
Pass IO:
    BackBuffer      RenderTarget
    MainDepth       DepthTarget

Common resources:
    PerFrame        UniformData<PerFrameConstantBufferData>
    PerView         UniformData<PerViewConstantBufferData>

Vertex resources:
    PerObjectVS     UniformData<PerObjectVSConstantBufferData>

Pixel resources:
    PerObjectPS     UniformData<PerObjectPSConstantBufferData>
    TextureBaseColor
    TextureNormal
    TextureMetallicRoughness
    TextureOcclusion
    TextureEmissive
    ShadowMap0
    ShadowMap1
    ShadowMap2
    ShadowMap3
    SamplerAniso16xWrap
    SamplerLinearNoMipClamp
```

Do not keep `MaterialTextures[5]` in the generated layout.

## ShadowOpaque target metadata

Target logical bindings:

```text
Pass IO:
    ShadowColor     RenderTarget
    ShadowDepth     DepthTarget

Common resources:
    PerFrame        UniformData<PerFrameConstantBufferData>
    PerView         UniformData<PerViewConstantBufferData>

Vertex resources:
    PerObjectVS     UniformData<PerObjectVSConstantBufferData>
```

## ComputeClear target metadata

Target logical bindings:

```text
Compute resources:
    Output          RWTexture / UAV
```

If HLSL currently uses `OutputTexture`, use the metadata alias form so the layout binding is `Output` while reflection validates the actual HLSL symbol.

## Deletion requirements

- Delete manual helper files after all call sites are removed.
- Remove includes of `BuiltinShaderPackageLayouts.h`.
- Remove stale manual layout declarations from CMake source lists if source files are explicitly listed.
- Search the workspace for `Build*ShaderPackageBindingLayout` and remove all remaining references.

## Acceptance criteria

- Built-in shader packages derive layouts from metadata.
- Manual helper files are deleted.
- No code path depends on manual package layout builders.
- Cooked package hashes are produced from generated layouts.
- ForwardOpaque uses individual material texture bindings.

## Validation

- Recook shaders for `Showcase Debug`.
- Build editor.
- Launch editor from the Showcase project working directory.
- Confirm cooked shader runtime ready logs for:
  - `ForwardOpaque`
  - `ShadowOpaque`
  - `ComputeClear`
- Search for deleted helper names and confirm zero references.
