<!-- ========================================================================= -->
<!-- TEXTURE INTEGRATION PLAN — Bindful Baseline Status                         -->
<!-- Full Sponza PBR Texture Support                                           -->
<!-- Sparkle Engine                                                            -->
<!-- ========================================================================= -->

# Texture Integration Plan - Bindful Baseline Status

**Parent:** [TEXTURE_PIPELINE_DESIGN.md](TEXTURE_PIPELINE_DESIGN.md)
**Status:** Active Baseline
**Date:** 2026-03-08

---

## 1. Purpose

This document now tracks the implemented bindful material-texture baseline rather than a future implementation plan.

The goal of this milestone was to establish an explicit, debuggable path for full glTF material texturing in Sponza without introducing bindless indexing or broader renderer redesign.

---

## 2. Implemented Baseline

### 2.1 Material Contract

The CPU-to-shader material contract is now in place.

- `MaterialDesc` carries:
  - `baseColor`
  - `metallic`
  - `roughness`
  - `f0`
  - `emissiveColor`
  - `alphaMode`
  - `alphaCutoff`
  - texture paths for albedo, normal, metallic-roughness, occlusion, and emissive
- `GltfLoader` extracts those fields from glTF material data.
- `MaterialData` carries scalar fallbacks, `TextureFlags`, alpha data, and the GPU handle for the material texture table.
- `PerObjectPSConstantBufferData` in C++ and HLSL is aligned with the expanded material payload.

### 2.2 Renderer Texture Runtime

The renderer now has a path-based material texture cache.

- `TextureManager::LoadFromPath(...)` resolves texture paths through the asset system.
- Cache keys are normalized canonical paths.
- Repeated paths reuse an already loaded texture object.
- Renderer-owned fallback policy is split from generic default textures:
  - `DefaultTextures` defines generic renderer defaults.
  - `MaterialFallbackTextures` maps material slots to default textures.
- All five material slots are expected to bind valid SRVs, including fallback textures when authored maps are missing.

### 2.3 GPU Upload Path

The resource layer now supports mip-capable texture payloads.

- `TexturePayload` carries width, height, DXGI format, format intent, and explicit mip data.
- `D3D12Texture` now builds texture resources from `TexturePayload` rather than from a file path.
- `D3D12Texture` creates resources and SRVs with full mip visibility based on the payload.
- The descriptor allocator can reuse freed contiguous SRV ranges, which is required for persistent per-material 5-SRV tables.

### 2.4 Bindful Material Binding

The renderer is now using an explicit fixed-slot bindful model.

- Root signature material texture range is `t0..t4`.
- Material texture slots are fixed:
  - `t0` Base Color
  - `t1` Normal
  - `t2` Metallic-Roughness
  - `t3` Occlusion
  - `t4` Emissive
- `Renderer::BuildMaterials(...)` builds one contiguous 5-SRV descriptor table per material.
- These tables are persistent and rebuilt only when the loaded material set changes.
- `ForwardOpaquePass` now binds the current material texture table per draw instead of binding the checker globally as the effective material source.

### 2.5 Shader Material Path

The forward shader path now understands the glTF material contract.

- base color texture or scalar fallback
- normal map sampling and unpacking
- metallic-roughness unpacking from `G/B`
- occlusion sampling from `R`
- emissive sampling multiplied by `EmissiveColor`
- alpha masking through `clip(alpha - AlphaCutoff)`
- authored alpha output for `Blend` materials

---

## 3. Actual Runtime Flow

The current implemented data flow is:

```text
glTF material
  -> GltfLoader extracts scalar values, alpha state, and texture paths
  -> Scene stores MaterialDesc
  -> Renderer compares the loaded material set against its cached copy
  -> Renderer rebuilds material texture tables only when that set changes
  -> TextureManager resolves each path to a cached runtime texture or fallback
  -> Renderer allocates one contiguous 5-SRV table per material
  -> SceneView references MaterialData entries that point at those tables
  -> ForwardOpaquePass binds the per-draw material table at RootParam::TextureSRV
  -> Material.hlsli samples t0..t4 with scalar and flag fallbacks
```

This is the active baseline for material texturing in the forward renderer.

---

## 4. Current Validation Status

### Verified in code

- Per-material descriptor tables are built and cached in the renderer.
- The forward pass binds the current material texture table per draw.
- The shader path samples all five material texture types.
- Missing authored maps resolve to fallback SRVs plus scalar and flag fallbacks.
- Repeated texture paths are cached by canonical path.

### Partially complete

- `Blend` material data reaches the shader and authored alpha is preserved in the forward pixel shader output.
- The resource upload path supports multiple mips if supplied in `TexturePayload`.

### Not yet complete for the full visual milestone

- Transparent materials are not fully supported yet at the pass and pipeline-state level.
- Runtime material texture loading still produces only a single mip level.
- The acceptance target of reduced shimmer at distance is therefore not met yet.

---

## 5. Important Accuracy Notes

### WIC status

WIC has not been removed from the active runtime texture path.

- `TextureManager` currently builds `TexturePayload` from `TextureLoader` output.
- `TextureLoader` still uses Windows Imaging Component.
- The new payload-based `D3D12Texture` path is in place, but the source image loading and mip-generation side has not yet been migrated to `stb_image` and `stb_image_resize2`.

This means the architecture is bindful and payload-based already, but the runtime image ingestion side remains transitional.

### Bindless status

Bindless remains explicitly out of scope.

- no descriptor indexing in shaders
- no global texture-array sampling model
- no bindless material indirection layer

---

## 6. Deferred Items

These items are intentionally deferred beyond the current bindful baseline.

- Runtime migration from WIC to `stb_image`
- Runtime mip generation via `stb_image_resize2`
- Transparent pass and blend-state completion for authored `Blend` materials
- BC compression pipeline
- KTX2 container pipeline
- Texture streaming
- Automated tests for the material texture path
- Performance work such as material sorting or more granular cache invalidation

---

## 7. Residual Risks

| Risk | Current State | Consequence |
|---|---|---|
| Single-mip runtime material loading | Still present | Shimmering and poor distance stability remain likely |
| Opaque-only forward pass structure | Still present | `Blend` materials are not fully correct yet |
| WIC still in the runtime path | Still present | Windows-only material texture ingestion remains in place |
| Material-cache invalidation only compares the loaded material set | Accepted for now | Future scene-edit workflows may need more granular invalidation |

---

## 8. Natural Next Steps

1. Replace the current WIC-backed material texture ingestion path with `stb_image` and generate full mip chains with `stb_image_resize2`.
2. Add a real transparent rendering path for `AlphaMode::Blend`, including blend-state enablement and draw ordering policy.
3. Add focused validation coverage for path-cache reuse, fallback SRV binding, material constant-buffer layout, and mip-count propagation.
4. Revisit compression, KTX2, and performance work only after the first correct visual baseline is complete.


