<!-- ========================================================================= -->
<!-- TEXTURE PIPELINE DESIGN — Bindful Material Texturing                       -->
<!-- Sparkle Engine - Architecture Decision Record                             -->
<!-- ========================================================================= -->

# Texture Pipeline Design - Bindful Material Texturing

**Author:** Engine Architecture
**Status:** Implemented Baseline
**Date:** 2026-03-08

---

## 1. Objective

Define the as-built texture architecture used to move glTF material data into the Sparkle forward renderer through an explicit bindful material path.

This document describes the current baseline, its ownership model, and the limitations that remain after the first bindful texture milestone.

Bindless is intentionally out of scope for this phase.

---

## 2. Design Decision

Sparkle uses one persistent fixed-slot SRV descriptor table per material.

This is the chosen binding model for the current engine stage because it:

- keeps resource ownership explicit
- matches the existing D3D12 root-binding style
- is easy to inspect from CPU material data to final draw submission
- avoids introducing shader descriptor indexing before the material path is fully stable

The current material texture table layout is fixed:

- `t0 = BaseColor`
- `t1 = Normal`
- `t2 = MetallicRoughness`
- `t3 = Occlusion`
- `t4 = Emissive`

---

## 3. Layer Responsibilities

### 3.1 GameFramework layer

Owns asset-facing material description only.

Primary type:

- `MaterialDesc`

Responsibilities:

- store scalar PBR values
- store alpha behavior
- store authored texture paths
- remain free of GPU handles and renderer objects

### 3.2 Renderer layer

Owns material packaging, fallback policy, caching, and descriptor-table lifetime.

Primary types and utilities:

- `MaterialData`
- `TextureManager`
- `DefaultTextures`
- `MaterialFallbackTextures`
- `MaterialCacheUtils`
- `Renderer`

Responsibilities:

- convert `MaterialDesc` to renderer-facing `MaterialData`
- resolve texture paths to cached runtime textures
- provide valid fallback textures for missing maps
- allocate and free per-material descriptor tables
- store persistent material-table handles and expose them to the draw path

### 3.3 RHI layer

Owns resource creation, descriptor allocation, and root-signature representation.

Primary types:

- `TexturePayload`
- `D3D12Texture`
- `D3D12DescriptorAllocator`
- `D3D12RootSignature`

Responsibilities:

- create texture resources from explicit payload data
- support multi-mip texture resources when the payload supplies them
- allocate contiguous SRV ranges for material tables
- expose a fixed root-signature contract for the forward renderer

### 3.4 Shader layer

Owns interpretation of the material contract.

Primary files:

- `ConstantBuffers.hlsli`
- `Material.hlsli`
- `ForwardLitPS.hlsl`

Responsibilities:

- consume `TextureFlags`, alpha state, and scalar fallbacks
- sample the fixed material slots `t0..t4`
- apply glTF channel semantics correctly
- preserve authored alpha for blend materials at shader output

---

## 4. Material Contract

### 4.1 CPU material description

`MaterialDesc` now includes:

- `baseColor`
- `metallic`
- `roughness`
- `f0`
- `emissiveColor`
- `alphaMode`
- `alphaCutoff`
- `albedoTexture`
- `normalTexture`
- `metallicRoughnessTexture`
- `occlusionTexture`
- `emissiveTexture`

`GltfLoader` is responsible for extracting these values from `cgltf_material`.

### 4.2 Renderer material package

`MaterialData` now includes:

- scalar fallbacks used for the per-object pixel constant buffer
- `TextureFlags`
- alpha mode and alpha cutoff
- `textureTableGpuHandle`

The renderer builds `MaterialData` from `MaterialDesc` and stores the bindful descriptor-table handle once the material cache is rebuilt.

### 4.3 Constant-buffer contract

`PerObjectPSConstantBufferData` is the scalar and flag contract shared between C++ and HLSL.

It includes:

- `BaseColor`
- `EmissiveColor`
- `Metallic`
- `Roughness`
- `F0`
- `AlphaCutoff`
- `AlphaMode`
- `TextureFlags`

This contract is the fallback path when a given authored map is absent.

---

## 5. Runtime Texture Architecture

### 5.1 Path-based cache

`TextureManager` maintains a material texture cache keyed by normalized resolved paths.

Behavior:

- resolve through the asset system
- normalize the result
- reuse already loaded textures on repeated paths
- keep engine fixed-slot textures and material path-loaded textures separate

### 5.2 Default and fallback policy

Default texture definitions and material-slot fallback policy are intentionally separated.

- `DefaultTextures` defines generic renderer defaults.
- `MaterialFallbackTextures` maps each material slot to a renderer-owned fallback.

Current fallback intent is:

- albedo -> white
- normal -> flat tangent normal
- metallic-roughness -> default non-metal / rough surface
- occlusion -> white
- emissive -> black

### 5.3 Upload model

`D3D12Texture` is no longer responsible for loading files directly.

Instead:

- `TexturePayload` carries texture dimensions, format intent, DXGI format, and mip data
- `D3D12Texture` consumes that payload to create the resource, upload subresources, and build the SRV

This separates image ingestion from GPU resource creation and keeps the design compatible with a future non-WIC image path.

---

## 6. Descriptor And Lifetime Model

The renderer owns persistent descriptor tables for materials.

Current policy:

- one contiguous 5-SRV table is allocated per material
- those tables live across frames
- tables are released and rebuilt when the loaded material set changes
- `SceneView` references `MaterialData`, but does not own descriptor resources

This keeps the draw path simple and avoids per-frame descriptor churn for stable scene content.

---

## 7. Draw-Time Binding Model

The forward pass uses the following model:

```text
bind frame/view constant buffers
bind sampler table
for each draw:
  bind per-object VS constant buffer
  bind per-object PS constant buffer
  bind material texture table at RootParam::TextureSRV
  draw
```

The important change from the previous baseline is that the checker texture is no longer the effective global material source during forward rendering.

---

## 8. Shader Interpretation Rules

The current shader path follows glTF packing and fallback rules.

- base color uses texture sample or scalar fallback
- normal uses authored tangent-space map when present
- metallic-roughness uses `G` for roughness and `B` for metallic
- occlusion uses `R`
- emissive texture is multiplied by `EmissiveColor`
- `AlphaMode::Mask` clips on `alpha - AlphaCutoff`
- `AlphaMode::Blend` preserves authored alpha in pixel output

This keeps the shader contract explicit and directly traceable to CPU material data.

---

## 9. Current Limitations

The bindful baseline is implemented, but the full visual milestone is not yet complete.

### 9.1 WIC still present in runtime ingestion

The active material texture-loading path still depends on `TextureLoader`, which uses Windows Imaging Component.

What is true today:

- the renderer cache and payload-based texture resource path are implemented
- the actual image decode path still comes from WIC-backed `TextureLoader`
- runtime migration to `stb_image` and `stb_image_resize2` has not happened yet

### 9.2 Full mip behavior not yet delivered

The resource layer can upload multiple mip levels, but the current runtime material-texture path only supplies a single base mip.

Consequence:

- the design supports mip chains
- the current runtime baseline does not yet realize the intended visual stability benefits

### 9.3 Blend rendering is not complete

The shader now preserves authored alpha for blend materials, but transparent rendering is still incomplete at the pipeline and pass level.

Consequence:

- `Blend` material data reaches the shader
- correct transparent composition still requires follow-up renderer work

---

## 10. Explicitly Deferred

The following remain out of scope for the current baseline:

- bindless texture arrays
- shader descriptor indexing
- BC compression pipeline
- KTX2 packaging pipeline
- texture streaming
- virtual texturing
- automated tests for this pipeline
- performance optimizations such as material sorting or more granular descriptor-cache invalidation

---

## 11. Decision Record

- Keep the material texture path bindful.
- Keep material texture slots fixed at `t0..t4`.
- Keep descriptor ownership in the renderer.
- Keep resource creation payload-based in the RHI.
- Treat the current WIC-backed material texture ingestion path as transitional rather than as the desired end state.
