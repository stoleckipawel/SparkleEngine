---
description: "Use when: implementing phase 3 of Sparkle shader layout automation: cook planner and reflection verification."
name: "Sparkle Shader Layout Automation Phase 3 Cook Verification"
argument-hint: "Switch ShaderCompiler cook to generated layouts"
agent: "agent"
---

# Phase 3 — Cook Integration and Reflection Verification

Switch the ShaderCompiler cook pipeline from manual package layout helpers to generated package layouts, and make reflection verification understand reflected vs layout-only metadata.

## Goal

During shader cook:

```text
metadata registry + compiled shader reflection -> verified canonical layout -> cooked binding records
```

Manual layout functions must no longer be the authority for cooked package binding records.

## Files to inspect first

- `Tools/ShaderCompiler/Private/Cooking/ShaderCookPlanner.cpp`
- `Tools/ShaderCompiler/Private/Verification/ShaderParameterStructVerifier.cpp`
- `Tools/ShaderCompiler/Private/Cooking/CookedPackageWriter.cpp`
- `Engine/RHI/Private/Shaders/CookedShaderPackageUtils.cpp`
- `Engine/RHI/Public/Shaders/Authoring/GlobalShader.h`
- `Engine/RHI/Public/Shaders/Authoring/ShaderParameterStruct.h`
- `Engine/RHI/Public/Shaders/ShaderPackageLayoutBuilder.h` or equivalent builder file from Phase 2

## Required cook behavior

Update `ShaderCookPlanner` so package descriptors get their binding layout from `ShaderPackageLayoutBuilder`, not per-shader manual callbacks.

Required process:

```text
Find package registrations
    |
    v
Build canonical package layout from metadata
    |
    v
Compile each shader stage
    |
    v
Extract reflection
    |
    v
Verify reflected metadata fields
    |
    v
Write cooked package records and hash
```

## Verification rules

Update `ShaderParameterStructVerifier`:

1. If `Reflected == true`, the field must match compiled shader reflection.
2. If `Reflected == false`, the field is layout-only and must not require shader reflection.
3. Extra reflected resources should be hard errors unless a documented temporary migration exception is necessary.
4. Type/domain/access/array mismatches are hard errors.
5. Cook must fail before writing a package when verification fails.

## Hard failure examples

Fail the cook for cases like:

```text
Shader package 'ForwardOpaque' field 'TextureBaseColor' is declared in metadata but missing from pixel shader reflection.
```

```text
Shader package 'ComputeClear' field 'Output' is declared as RWTexture but reflection reports Texture2D SRV.
```

```text
Shader package 'ForwardOpaque' has unexpected reflected resource 'MaterialTextures'. Declare it or remove it from HLSL.
```

## Cooked record requirements

- `CookedShaderBindingRecord` stays backend-neutral.
- `BindingLayoutHash` is computed from the generated canonical `PassParameterLayout`.
- Do not serialize D3D12 root parameters or descriptor registers as the authoritative cooked ABI.

## Acceptance criteria

- Shader cook uses generated layouts.
- Layout-only pass IO no longer breaks reflection verification.
- Reflected resources are validated against compiled shader reflection.
- Cook fails hard on metadata/reflection mismatch.
- Existing manual layout helper files may still exist but are no longer used by the cooker.

## Validation

- Run shader cook for `Showcase Debug` using the repository shader cook flow.
- Inspect logs for generated layout diagnostics.
- Confirm cooked `BindingLayoutHash` is deterministic across repeated cooks with no source changes.
- Do not proceed to runtime traits cleanup until cook can produce valid packages.
