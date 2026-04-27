---
description: "Use when: implementing phase 5 of Sparkle shader layout automation: individual material textures and shared/unique sampler policies."
name: "Sparkle Shader Layout Automation Phase 5 Binding Policies"
argument-hint: "Implement material texture slices and sampler policy"
agent: "agent"
---

# Phase 5 — Material Texture and Sampler Binding Policies

Implement the runtime binding model required by generated shader layouts:

- individual material texture bindings,
- descriptor table slice overrides,
- shared sampler and unique sampler support.

## Goal

Replace the old logical binding:

```text
MaterialTextures[5]
```

with named shader bindings:

```text
TextureBaseColor
TextureNormal
TextureMetallicRoughness
TextureOcclusion
TextureEmissive
```

The material system may still allocate one descriptor table internally; the shader ABI should expose individual logical names.

## Files to inspect first

- `Engine/Renderer/Private/Passes/ForwardOpaquePass.cpp`
- `Engine/Renderer/Private/Pipeline/PassBindingOverrides.h`
- `Engine/Renderer/Private/Pipeline/PassBindingOverrides.cpp`
- `Engine/Renderer/Private/Pipeline/PassBinder.cpp`
- `Engine/Renderer/Private/Pipeline/RenderPassPipelineTraits.h`
- `Engine/Renderer/Private/SceneData/MaterialData.h`
- `Engine/Renderer/Private/Materials/MaterialCacheManager.cpp`
- `Engine/RHI/Public/Interop/RenderHardwareInterface.h`
- `Engine/RHI/Private/D3D12/*`
- `Engine/RHI/Public/Shaders/Authoring/ShaderParameterStruct.h`

## Material texture data flow

Implement this conceptual flow:

```text
MaterialCacheManager
    creates descriptor table with material texture slots
        slot 0 TextureBaseColor
        slot 1 TextureNormal
        slot 2 TextureMetallicRoughness
        slot 3 TextureOcclusion
        slot 4 TextureEmissive
        |
        v
MaterialData::textureTableHandle
        |
        v
ForwardOpaquePass::DrawOpaqueMeshes
        |
        |-- bind TextureBaseColor from slice 0
        |-- bind TextureNormal from slice 1
        |-- bind TextureMetallicRoughness from slice 2
        |-- bind TextureOcclusion from slice 3
        |-- bind TextureEmissive from slice 4
        v
PassBinder resolves logical names to backend descriptor bindings
```

## Required API shape

Add an RHI-neutral descriptor table slice binding concept. Suggested shape:

```cpp
overrides.SetDescriptorTableSlice("TextureBaseColor", tableHandle, MaterialTextureSlots::BaseColor);
```

or an equivalent strongly typed API that avoids exposing D3D12-native handles outside the backend.

## Sampler policy

Support both:

```cpp
SHADER_PARAMETER_SHARED_SAMPLER(SamplerAniso16xWrap)
SHADER_PARAMETER_SHARED_SAMPLER(SamplerLinearNoMipClamp)
```

and future unique sampler declarations:

```cpp
SHADER_PARAMETER_UNIQUE_SAMPLER(SamplerState, MySampler)
```

Shared sampler flow:

```text
logical sampler name -> global sampler table entry -> backend descriptor/static sampler
```

Unique sampler flow:

```text
shader metadata + reflection -> pass override or default sampler desc -> backend unique sampler binding
```

## Constraints

- Do not switch to bindless as the near-term material texture solution.
- Do not collapse individual material textures back into a generated `MaterialTextures[5]` binding.
- Keep descriptor table slice abstraction RHI-neutral.
- Do not make renderer code depend on D3D12 descriptor handle internals.

## Acceptance criteria

- Forward opaque draw code binds individual texture names.
- Existing material descriptor table allocation can remain, but the pass binding ABI is individual.
- Shared sampler names can be resolved and bound.
- The model leaves room for unique samplers without redesigning metadata again.

## Validation

- Recook shaders after declarations are migrated.
- Build editor.
- Launch editor and inspect logs for binding layout creation errors.
- If possible, inspect a graphics capture to confirm material textures bind to the expected shader slots.
