---
description: "Use when: implementing phase 4 of Sparkle shader layout automation: runtime generated package layout lookup and RenderPassPipelineTraits cleanup."
name: "Sparkle Shader Layout Automation Phase 4 Runtime Layouts"
argument-hint: "Update runtime layout lookup and RenderPassPipelineTraits"
agent: "agent"
---

# Phase 4 — Runtime Generated Layouts

Update runtime shader package loading and render pass traits so runtime no longer relies on manual package layout helper functions.

## Goal

Runtime should validate cooked packages against generated canonical layouts and create backend binding layouts from cooked records.

Target flow:

```text
RenderPassPipelineTraits
    |
    v
BuildRegisteredShaderPackageLayout(packageId)
    |
    v
expected BindingLayoutHash
    |
    v
CookedShaderPackageCache validates loaded package
    |
    v
RHI backend creates native binding layout
```

## Files to inspect first

- `Engine/Renderer/Private/Pipeline/RenderPassPipelineTraits.h`
- `Engine/RHI/Public/Shaders/BuiltinShaderPackageLayouts.h`
- `Engine/RHI/Private/Shaders/BuiltinShaderPackageLayouts.cpp`
- `Engine/RHI/Public/Shaders/Authoring/GlobalShader.h`
- `Engine/RHI/Private/Shaders/ShaderAuthoring.cpp`
- `Engine/RHI/Public/Shaders/ShaderPackageLayoutBuilder.h` or equivalent
- `Engine/RHI/Private/Shaders/CookedShaderPackageCache.cpp`
- `Engine/RHI/Private/Shaders/CookedShaderPackageUtils.cpp`
- `Engine/RHI/Private/D3D12/Pipeline/D3D12BindingLayout.cpp`

## Required runtime API

Add a runtime-safe helper that can build a package layout by package id from the registered shader metadata.

Suggested shape:

```cpp
bool BuildRegisteredShaderPackageLayout(
    std::string_view packageId,
    PassParameterLayout& outLayout,
    std::string& outErrorMessage);
```

Use existing error/fail/logging conventions.

## RenderPassPipelineTraits changes

Update traits for:

- `ForwardOpaquePass`
- `ShadowOpaquePass`
- `ComputeClearPass`

They should request generated package layouts instead of calling:

- `BuildForwardOpaqueShaderPackageBindingLayout()`
- `BuildShadowOpaqueShaderPackageBindingLayout()`
- `BuildComputeClearShaderPackageBindingLayout()`

Do not delete the manual helper files in this phase if Phase 6 has not migrated all built-in shader declarations yet.

## Runtime validation behavior

- If generated expected layout fails, fail startup with an actionable message.
- If cooked `BindingLayoutHash` does not match generated expected hash, fail startup.
- Do not add runtime shader compilation fallback.
- Do not silently accept stale cooked packages.

## Backend behavior

- D3D12 binding layout creation should still consume `CookedShaderBindingRecord[]`.
- Render/depth targets may be present in package ABI but skipped for root signature generation where appropriate.
- Keep backend-specific root signature decisions in the D3D12 implementation.

## Acceptance criteria

- Runtime pass traits no longer duplicate package layout definitions.
- Runtime package hash validation uses generated expected layouts.
- Existing cooked package cache behavior remains strict.
- No backend-neutral code depends on D3D12 root signature details.

## Validation

- Recook shaders after Phase 3 if needed.
- Build editor.
- Launch editor from the Showcase project working directory.
- Confirm logs show cooked shader runtime ready for:
  - `ForwardOpaque`
  - `ShadowOpaque`
  - `ComputeClear`
