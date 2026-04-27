---
description: "Use when: implementing phase 2 of Sparkle shader layout automation: canonical package layout builder."
name: "Sparkle Shader Layout Automation Phase 2 Package Builder"
argument-hint: "Implement ShaderPackageLayoutBuilder only"
agent: "agent"
---

# Phase 2 — Canonical Shader Package Layout Builder

Implement the package-level layout builder. Do not switch the cook pipeline or runtime pipeline yet unless a minimal call site is needed for compile coverage.

## Goal

Create a single canonical path that converts registered shader/pass metadata into `PassParameterLayout` for one shader package.

## Files to inspect first

- `Engine/RHI/Public/Shaders/Authoring/GlobalShader.h`
- `Engine/RHI/Private/Shaders/ShaderAuthoring.cpp`
- `Engine/RHI/Public/Shaders/Authoring/ShaderParameterStruct.h`
- `Engine/RHI/Public/ShaderParameters/PassParameterLayout.h`
- `Engine/RHI/Private/Shaders/CookedShaderPackageUtils.cpp`
- `Tools/ShaderCompiler/Private/Cooking/ShaderCookPlanner.cpp`

## New API target

Add an RHI-level package layout builder. Suggested shape:

```cpp
class ShaderPackageLayoutBuilder final
{
  public:
    static bool Build(
        std::string_view packageId,
        std::span<const ShaderRegistrationDesc> registrations,
        PassParameterLayout& outLayout,
        std::string& outErrorMessage);
};
```

Adapt naming and ownership to existing code. Prefer declarations in headers and non-template definitions in `.cpp` files.

## Merge rules

The builder must:

1. Collect all shader stages registered for one package.
2. Collect pass/package IO metadata if available in this phase.
3. Generate a deterministic `PassParameterLayout`.
4. Merge duplicate `LayoutName` entries only when compatible:
   - same semantic kind,
   - same resource domain,
   - same access policy,
   - same array count,
   - same value size/alignment for uniform data.
5. OR compatible stage visibility masks.
6. Fail with a clear diagnostic for incompatible duplicate logical names.
7. Preserve backend neutrality. Do not assign D3D12 registers or root parameter indices here.

## Ordering policy

Use deterministic ordering. Recommended initial order:

```text
1. pass IO fields: render targets and depth targets
2. package-level/global resources: per-frame, per-view, shadow maps
3. stage/draw resources: per-object data, material textures, UAVs
4. samplers
```

If the existing registry order is easier and stable, use it, but document the choice in comments. The same input metadata must always produce the same hash.

## Diagnostics

Create readable error messages such as:

```text
Shader package 'ForwardOpaque' has incompatible binding 'PerView':
  Vertex shader declares UniformData<PerViewConstantBufferData>, visibility Vertex
  Pixel shader declares ReadTexture, visibility Pixel
```

## Acceptance criteria

- A package layout can be generated from metadata without invoking manual `Build*ShaderPackageBindingLayout()` helpers.
- Duplicate compatible names merge visibility.
- Duplicate incompatible names fail with an actionable error.
- Existing code still builds.
- Manual helper files remain untouched in this phase unless unavoidable.

## Validation

- Add a small unit/self-test if the project has a suitable validation target.
- Otherwise add focused compile coverage and clear diagnostics.
- Do not recook shaders as the primary validation until Phase 3 switches the cook path.
