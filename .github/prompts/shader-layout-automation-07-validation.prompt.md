---
description: "Use when: validating and hardening Sparkle shader layout automation after migration."
name: "Sparkle Shader Layout Automation Phase 7 Validation"
argument-hint: "Validate generated shader layouts and add regression guards"
agent: "agent"
---

# Phase 7 — Validation, Diagnostics, and Regression Guards

Validate the full shader layout automation migration and add guardrails so manual layout duplication does not return.

## Goal

Prove the final pipeline works:

```text
UE-style shader/pass parameter macros
    |
    v
metadata registry
    |
    v
generated package layout
    |
    v
reflection validation
    |
    v
cooked backend-neutral binding records
    |
    v
runtime backend binding layout
```

## Validation checklist

### Static searches

Search for and remove or justify any remaining references to:

```text
BuildForwardOpaqueShaderPackageBindingLayout
BuildShadowOpaqueShaderPackageBindingLayout
BuildComputeClearShaderPackageBindingLayout
BuildPackageBindingLayout
BuiltinShaderPackageLayouts
MaterialTextures
```

`MaterialTextures` may remain only in old comments/docs explicitly describing removed behavior.

### Cook validation

- Recook `Showcase Debug` shaders.
- Confirm generated layout diagnostics include expected package bindings.
- Confirm `ForwardOpaque` no longer contains `MaterialTextures[5]`.
- Confirm `BindingLayoutHash` is deterministic across repeated cooks.

### Runtime validation

- Build editor.
- Launch from the Showcase project working directory.
- Confirm logs show cooked shader runtime ready for:
  - `ForwardOpaque`
  - `ShadowOpaque`
  - `ComputeClear`
- Confirm no `BindingLayoutHash expected/actual` failure.

### Negative validation

Add or run a test/validation path that proves bad metadata fails loudly. Examples:

1. Duplicate logical binding with incompatible type.
2. Metadata field missing from HLSL reflection.
3. Reflected HLSL resource missing from metadata.
4. `RWTexture` declared as read-only texture.
5. Layout-only pass IO incorrectly sent through reflection verification.

## Diagnostics to add if missing

Add a shader compiler inspect command or log mode that prints:

```text
Package: ForwardOpaque
Variant: Default
BindingLayoutHash: ...

Bindings:
  BackBuffer                 RenderTarget    AllGraphics   reflected=false
  MainDepth                  DepthTarget     AllGraphics   reflected=false
  PerFrame                   UniformData     AllGraphics   reflected=true
  PerView                    UniformData     AllGraphics   reflected=true
  PerObjectVS                UniformData     Vertex        reflected=true
  PerObjectPS                UniformData     Pixel         reflected=true
  TextureBaseColor           ReadTexture     Pixel         reflected=true
  TextureNormal              ReadTexture     Pixel         reflected=true
  TextureMetallicRoughness   ReadTexture     Pixel         reflected=true
  TextureOcclusion           ReadTexture     Pixel         reflected=true
  TextureEmissive            ReadTexture     Pixel         reflected=true
```

## Regression guard options

Choose at least one:

1. Build-time static check that fails if `BuiltinShaderPackageLayouts` or `Build*ShaderPackageBindingLayout` is reintroduced.
2. ShaderCompiler self-test for package layout merge failures.
3. Cook validation test fixture with intentionally bad shader metadata.
4. CI boundary check that prevents renderer code from depending on D3D12 descriptor internals.

## Acceptance criteria

- Generated layout path is validated by cook, build, and editor startup.
- Manual layout helper path is absent.
- Diagnostics make layout/hash mismatches actionable.
- At least one regression guard prevents the old duplication pattern from returning.
- Repository memory or architecture docs record the final ownership decision.
