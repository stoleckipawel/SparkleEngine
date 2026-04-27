---
description: "Use when: starting Sparkle shader layout automation; selects the correct phase prompt and preserves architectural decisions."
name: "Sparkle Shader Layout Automation Phase Index"
argument-hint: "Phase number or current implementation status"
agent: "agent"
---

# Sparkle Shader Layout Automation Phase Index

You are implementing Sparkle's shader layout automation plan. Do not implement all phases in one pass unless explicitly asked. Use this prompt to identify the correct next phase, then follow that phase-specific prompt.

## Final architecture decisions

- Replace manual `PassParameterLayout Build*ShaderPackageBindingLayout()` code with generated package layouts.
- Use UE-style explicit shader/pass parameter macros as the authoring surface.
- Use hybrid metadata + reflection validation:
  - C++ metadata declares engine intent.
  - Shader reflection proves compiled HLSL agrees.
- Pass IO belongs in pass parameter metadata:
  - render targets
  - depth targets
  - imported/transient pass resources
- Material textures are individual named bindings:
  - `TextureBaseColor`
  - `TextureNormal`
  - `TextureMetallicRoughness`
  - `TextureOcclusion`
  - `TextureEmissive`
- Samplers must support both policies:
  - shared/global samplers
  - unique/reflected samplers
- Cook failures are hard failures. Do not add runtime shader compile fallback.
- The migration applies to the whole shader system, not just the three built-in packages.
- Keep cooked binding records backend-neutral. D3D12/Vulkan translation stays backend-private.

## Phase prompts

Run these in order unless the codebase already contains an equivalent phase:

1. `/Sparkle Shader Layout Automation Phase 1 Metadata`
   - Extend parameter metadata.
   - Add UE-style explicit macros.
   - Preserve old macros only as temporary wrappers.

2. `/Sparkle Shader Layout Automation Phase 2 Package Builder`
   - Add `ShaderPackageLayoutBuilder`.
   - Merge all shader stage descriptors for a package.
   - Produce canonical `PassParameterLayout`.

3. `/Sparkle Shader Layout Automation Phase 3 Cook Verification`
   - Update `ShaderCookPlanner` and `ShaderParameterStructVerifier`.
   - Generate layout during cook.
   - Validate reflected fields and fail hard on mismatches.

4. `/Sparkle Shader Layout Automation Phase 4 Runtime Layouts`
   - Add runtime generated-layout lookup.
   - Update `RenderPassPipelineTraits` to stop consuming manual helpers.

5. `/Sparkle Shader Layout Automation Phase 5 Binding Policies`
   - Add individual material texture binding flow.
   - Add shared and unique sampler policies.

6. `/Sparkle Shader Layout Automation Phase 6 Builtin Migration`
   - Migrate `ForwardOpaque`, `ShadowOpaque`, and `ComputeClear`.
   - Delete manual layout helper files and overrides.

7. `/Sparkle Shader Layout Automation Phase 7 Validation`
   - Recook, build, launch, inspect hashes, and add regression guards.

## Current-to-target mental model

Current:

```text
Shader params + manual PassParameterLayout helper
        |
        v
Cook uses manual helper
        |
        v
Runtime repeats expected manual layout
        |
        v
D3D12 root signature
```

Target:

```text
Shader/pass parameter macros
        |
        v
Generated metadata registry
        |
        v
Package layout builder + reflection validation
        |
        v
Cooked backend-neutral binding records
        |
        v
Runtime backend layout creation
```

## Must-not rules

- Do not keep compatibility shims once the new architecture is validated.
- Do not add bindless as the near-term material texture solution.
- Do not add runtime shader compilation fallback.
- Do not encode D3D12 root signature details into cooked package records.
- Do not hide behavior behind naming conventions when explicit metadata is available.
- Do not rewrite unrelated renderer/RHI code while implementing a phase.

## Before each phase

1. Read the phase prompt fully.
2. Inspect current files before editing; user may have changed them.
3. Build the smallest working vertical slice for that phase.
4. Validate with focused compiler checks or builds when practical.
5. Record any new architectural facts in repository memory if they will affect later phases.
