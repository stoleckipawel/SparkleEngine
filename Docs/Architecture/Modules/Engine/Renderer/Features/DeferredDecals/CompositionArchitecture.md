# Deferred GBuffer Decal Composition Architecture

Status: target architecture; not implemented behavior
Date: 2026-08-17
Last source reconciliation: 2026-08-28 at committed `master` revision `20814381`; source and executable build configuration are unchanged from implementation revision `99af6d5b`
Scope: projected material decals on opaque and alpha-tested GBuffer receivers, raster and ray-traced primary visibility, later arbitrary ray hits used by GI and reflections, D3D12/Vulkan parity, authoring, and ownership

## Decision

Sparkle should implement decals as material overlays, never as a forward-lit color effect:

1. Rasterized and ray-traced primary visibility continue to produce the same base GBuffer.
2. One `DeferredDecalResolve` compute pass runs after either producer and before depth-derived and lighting consumers.
3. That pass reads the receiver depth and GBuffer, evaluates the visible decals for each covered pixel, and writes the composed material values back to the existing GBuffer.
4. Later GI and reflection rays apply the same projection, sampling, ordering, and material-composition functions at an arbitrary world-space hit. Only candidate lookup differs.
5. Decals reuse ordinary materials, the existing scene material texture table, `RenderObjectId`, frame-graph lifetime, and persistent GPU-scene update patterns. There is no decal material cache, forward fallback, DBuffer, second TLAS, or parallel ray material system.

This is a target design. Code, cooked schemas, build configuration, and executable tests remain the authority for what exists today.

This document owns decal semantics, data flow, pass placement, and shared raster/ray composition. The adjacent [feature acceptance contract](Acceptance.md) owns the validation fixture, controlled failures, checks, and completion gates; the [Deferred GBuffer Decals Delivery Plan](../../../../../../Plans/Renderer/DeferredGBufferDecals.md) owns feature-local phase order. [Shader System Architecture](../../../../../CrossModule/ShaderSystem/README.md) owns shader identity and publication; the [Shader System Delivery Plan](../../../../../../Plans/CrossModule/ShaderSystem.md) owns the cross-system native RT pipeline/SBT/RHI sequence. The [Ray-Tracing Execution Architecture](../RayTracing/ExecutionArchitecture.md) owns the enduring inline-versus-pipeline and SBT semantics; adding decals must not create a third execution contract.

## Outcome

The completed path supports box-projected, softly blended decals that alter base color, world normal, metallic/roughness/AO/F0, emissive, and subsurface material values before lighting. The same authored decal can therefore appear:

- on a rasterized GBuffer receiver;
- on a primary surface written by either ray-tracing GBuffer frontend;
- in a later ray-traced reflection;
- in indirect-light material evaluation and bounce response.

It deliberately does not implement forward decals, mesh decals, transparent-surface decals, displacement, decal-cast geometry shadows, or emissive-decal light sampling. Those are separate capabilities and must not be implied by this architecture.

## Implementation Snapshot

The current source inventory, existing GBuffer/material seams, and absent decal path live in the [Deferred Decals feature dossier](README.md). Refresh that dossier before beginning a delivery phase; this architecture does not claim the path is implemented.

## Requirements And Boundaries

### Required

- Deferred GBuffer composition only; all lighting consumes the resulting material.
- Identical authored data and composition semantics for raster and ray-traced use.
- Stable overlap ordering across D3D12, Vulkan, raster primary, ray-traced primary, and secondary ray hits.
- Static box-projector decals suitable for the Modern Sponza short validation loop.
- Receiver opt-out without a general-purpose layer/filter framework.
- Zero decal content means zero decal pass, zero tile-list upload, and unchanged GBuffer output.
- Source-located validation failures for invalid material references, transforms, masks, and capacity.
- Existing frame-graph markers, captures, and GBuffer view modes remain the primary debugging surfaces.

### Not In The First Delivery

- Forward or scene-color decal fallback.
- Decals on alpha-blended/translucent surfaces, because those surfaces do not own a complete GBuffer receiver record.
- Mesh decals or topology-following projection.
- Animated projectors, skinned receiver candidate updates, or temporally correct moving-decal motion vectors.
- Per-channel blend modes, stain/modulate modes, height blending, parallax, or displacement.
- A dedicated decal acceleration structure, procedural AABB BLAS, or second TLAS.
- A decal editor panel, decal-specific material graph, runtime feature CVar, or per-frame log stream.

The first content is static by contract. Dynamic decals must not be advertised until motion, candidate invalidation, and temporal history behavior have their own accepted feature extension.

## External Precedent

[Deferred Decal Composition Precedent](../../../../../../Research/GraphicsArchitecture/DeferredDecalCompositionPrecedent.md) owns the Epic, Frostbite, i3D, Ray Tracing Gems II, and Intel findings that informed this design. The local choices below remain Architecture authority: projected ordered volumes, programmable pre-lighting GBuffer composition, receiver candidate spans before a dedicated decal AS, and a clearly labeled Sparkle-authored workload fixture.

## One Decal Contract

### Authored Value

Add one first-class `Decal` component to the existing ECS and cooked scene path. Its world transform maps a canonical unit box into the projection volume; scale is the volume size, so extents are not repeated in another field.

Conceptually, the component contains:

```text
Decal
  Material       ordinary MaterialHandle
  Opacity        [0, 1]
  SortOrder      signed authored layer
  ChannelMask    BaseColor | Normal | Material | Emissive | Subsurface
  FacingFade     start/end cosine
  EdgeFade       local depth-edge width
```

The projector points along local `+Z`. A receiver faces the projector when its original world normal points toward local `-Z`. Local `XY` maps to decal UV and local `Z` supplies depth/edge fade. The implementation must freeze this convention in cooker tests and an editor gizmo before content is authored.

`Material` resolves through the ordinary material table. The sampled base-color alpha, including its material factor, supplies texture coverage. Existing PBR constants and texture slots supply every enabled channel. `AlphaMode` does not choose a decal render path and must not create a second material type.

One receiver bit, `ReceivesDecals`, is added to mesh-instance data and defaults to true. The raster GBuffer writes it into the currently unused `GBufferNormal.w`; the ray hit instance carries the equivalent flag. Sky and invalid pixels store false. No layer-mask taxonomy is added until a real scene needs more than receive/ignore.

### Runtime Identity And Lifetime

Decals use the existing `RenderObjectId`, world transform, visibility, create/update/destroy sequence, and render-scene generation. Evolve `RenderObjectStaticData` into an explicit mesh-or-decal payload rather than creating a second object delta protocol. An entity that attempts to publish both payload kinds fails validation until a real combined-object use case exists.

The Renderer owns:

- one scene-owned decal table projected into frame-slot `PreparedRenderScene` data;
- one persistent structured GPU `DecalData` buffer updated through the existing dirty-range pattern;
- one per-view primary tile plan;
- later, one flat secondary-ray candidate index buffer and a span in each ray hit instance.

The GPU decal record contains only stable indices and POD: world-to-decal transform, projector basis or the data needed to derive it, material slot, opacity/fades, channel mask, sort order, and object-id tie-break value. It contains no descriptors, native handles, owning pointers, or duplicated material constants.

### Deterministic Order

All producers sort bottom-to-top by:

```text
(SortOrder, RenderObjectId.Value)
```

The object id is a tie-breaker, not an authoring layer. Candidate builders preserve this order, and shader loops apply indices in that order. D3D12 and Vulkan tests must use the same integer ordering. There is no state-sort reorder that changes visual layering.

## Shared Material Composition

The reusable boundary is the surface contract, not the traversal algorithm:

```text
base material fetch ------------+
                                 |
decal candidate -> projection -> sample -> CompositeDecal -> SurfaceMaterial
                                 |                     |
raster/primary RT: GBuffer ------+                     +-> pack GBuffer
secondary RT: hit surface -------+                     +-> shade ray hit
```

Introduce one shader value containing the material fields needed by both decoded GBuffer data and ray-hit shading:

```text
SurfaceMaterial
  BaseColor, Alpha
  NormalWorld
  Metallic, Roughness, AmbientOcclusion, DielectricF0
  Emissive
  SubsurfaceColor, SubsurfaceStrength
```

Ray geometry data such as position, previous position, tangent frame, UV, material slot, and rejection reason remains outside that value. This separation lets `RayTracingHitSurfaceData` contain a `SurfaceMaterial` instead of maintaining another flat copy of the same PBR fields. GBuffer decode/pack and ray material fetch remain producer adapters around the same value.

The common decal shader code owns only three operations:

1. `EvaluateDecalProjection`: exact unit-box containment, UV, edge fade, and facing fade from world position and the original receiver normal.
2. `SampleDecalMaterial`: fetch ordinary material constants and the existing global material texture table using explicit gradients or an explicit LOD supplied by the caller.
3. `CompositeDecal`: apply the selected semantic groups to an in/out `SurfaceMaterial`.

Coverage is:

```text
saturate(Opacity * SampledBaseColorAlpha * EdgeFade * FacingFade)
```

Composition rules are intentionally small:

| Group | Rule when enabled |
|---|---|
| BaseColor | Linear interpolation by coverage; preserve receiver `Alpha`. |
| Normal | Transform the sampled tangent normal through a projector-derived frame orthogonalized against the original receiver normal; normalized interpolation by coverage. |
| Material | Linear interpolation of metallic, roughness, AO, and dielectric F0 by coverage. |
| Emissive | Linear material-layer interpolation by coverage. Additive emissive is not an implicit special case. |
| Subsurface | Linear interpolation of color and strength by coverage. |

The original receiver normal controls facing and eligibility for every overlap. Previously applied decal normals do not change whether the next decal is accepted. The current accumulated surface receives the blend, so sort order remains meaningful.

The first version has one coverage and five semantic channel groups. It does not expose separate channel opacity, blend operators, or material-graph hooks. Add one only when an accepted asset cannot be expressed by this contract.

## Raster And Primary-Ray GBuffer Resolve

### Frame order

The selected insertion point is the existing join after the GBuffer producer:

```text
Create GBuffer targets
        |
        +-- RasterizedGBuffer VS/PS --------+
        |                                    |
        `-- RayTracing GBuffer Inline CS or Pipeline RGS +
                                             |
                              DeferredDecalResolve CS
                                             |
                              Sky motion / depth linearization
                                             |
                              Lighting / GI / reflections / post
```

The pass modifies BaseColor, Normal, Material, Emissive, and Subsurface as UAVs. It samples DeviceZ and leaves DeviceZ and MotionVector unchanged. The frame graph owns render-target/depth-to-SRV/UAV transitions and the transition back to lighting reads.

This placement is producer-independent: raster and ray-traced primary visibility must exercise the same shared pass. Do not add a temporary duplicate ray decal pass or a mode-specific shader; the delivery plan owns which producer is proven first.

### One dispatch, no pixel races

Do not dispatch once per decal and do not scan every decal at every screen pixel. Build a compact per-view plan on the CPU:

```text
sorted visible decals
       |
       +-> conservative projected screen bounds
       |
       `-> fixed-size screen tiles
              ActiveDecalTiles  { tileCoord, firstIndex, count }
              DecalTileIndices  { decalIndex... in stable layer order }
```

Bounds clipping must handle near-plane intersection and the camera inside a decal conservatively; an uncertain projection expands coverage and relies on the exact shader box test. It must never drop a valid pixel.

One compute group owns one active tile. One thread owns one pixel, loads the receiver GBuffer once, walks that tile's ordered candidates, and stores the composed GBuffer once. This gives:

- no UAV race between overlapping decals;
- no barrier or graph node per decal;
- no full-screen work when decals cover only a small region;
- stable layering independent of command scheduling;
- one visible frame-graph/GPU marker.

The tile size is a compile-time implementation constant selected from measurement, not a user CVar. If CPU binning becomes measurable, replace it with one demonstrated better owner and delete the old builder; do not retain two selectable planners.

### Per-Pixel Sequence

For an active pixel:

1. Load DeviceZ. Reject sky/background using the existing depth convention.
2. Load the five material GBuffer values and the `ReceivesDecals` bit. Reject a non-receiver.
3. Reconstruct unjittered world position from the pixel center, DeviceZ, and current view transforms.
4. Preserve the original receiver normal and receiver alpha.
5. For each ordered tile candidate, transform the position into projector-local space and reject outside the unit box or facing interval.
6. Compute UV and explicit gradients suitable for `SampleGrad`. Reconstruct neighboring positions and validate their depth/normal continuity; at a discontinuity, use a conservative footprint fallback rather than deriving across another surface or a volume edge.
7. Sample the ordinary material and call the shared composition function.
8. Repack the five GBuffer products, preserving receiver alpha and the receiver bit.

The pass must use the same jitter convention as the GBuffer depth sample. A projection test with a moving jitter sequence is an acceptance gate, not a late visual tweak.

## Arbitrary Ray Hits For GI And Reflections

A screen tile cannot answer which decals overlap a reflection hit outside the camera frustum. Secondary-ray support therefore shares decal data and evaluation but uses a view-independent candidate lookup.

### Initial Lookup

During ray-scene preparation, conservatively intersect each traceable receiver's world bounds with decal oriented-box bounds. Emit candidate decal indices in the same stable order into one flat buffer:

```text
RayTracingHitInstance
  ...existing geometry/material fields...
  FirstDecalCandidate
  DecalCandidateCount

RayTracingDecalCandidateIndices
  decalIndex, decalIndex, ...
```

At a shaded hit, the shader walks only that receiver span, performs the exact projection/facing test at the hit position, samples the same material table, and calls the same composition function.

Secondary texture filtering must receive an explicit surface footprint. If the ray path still uses the current fixed LOD-zero material sampling when secondary-ray decal work begins, introduce one reusable ray-cone/differential or conservative explicit-LOD contract for ordinary ray-hit materials and decals together. Do not add a decal-only filtering model, and do not accept a reflection path that aliases because every decal sample silently uses mip zero.

The first implementation may rebuild the flat candidate product when decal topology/transforms or receiver bounds change. That is correct for the static initial contract. Candidate count per receiver, total links, rebuild CPU time, and buffer high-water are delivery evidence fields; they are not a permanent UI product.

### Integration Rule

Keep base hit reconstruction separate from decal application:

```text
ReconstructRayTracingHitSurface
ApplyDecalsAtRayHit
ShadeRayTracingHitSurface
```

Call the middle operation for GI, reflection, and path-lighting material hits. Do not call it for:

- primary `RayTracingGBuffer`, because the shared screen resolve owns primary decals;
- shadow/visibility rays, because this decal contract changes material values, not geometry or alpha-test opacity;
- misses or invalid surfaces.

Before secondary-ray decals are accepted, the implementation must inventory every `ReconstructRayTracingHitSurface` consumer and classify it explicitly as primary material, arbitrary shaded material, shadow/visibility, or unsupported. No effect gets a private decal implementation.

### Why No Decal TLAS First

A procedural AABB acceleration structure can enumerate arbitrary overlaps and remains a valid measured alternative. It is not the initial choice because Sparkle currently owns triangle BLAS/TLAS products, while procedural decal geometry would add RHI geometry contracts, build inputs, hit-group/pipeline considerations, another lifetime product, and backend validation before proving that Modern Sponza needs them.

Adopt a dedicated decal AS only when receiver spans fail a recorded candidate-count or frame-time budget on the accepted workloads. If adopted, it replaces the span lookup while preserving the authored data and shared shader composition. It must not become a second decal system.

## Ownership

| Owner | Owns | Must not own |
|---|---|---|
| GameFramework scene/cooker | `Decal` component and current cooked schema, ordinary material reference, authored transform and parameters, source validation, stale-output rejection | GPU descriptors, frame order, GBuffer formats |
| Existing world extraction | Decal publication through the same object identity, transform, visibility, and delta lifecycle as meshes | A decal-only identity map or parallel scene snapshot |
| Renderer `RenderScene` | Single mutable decal authority and revision | Imported asset parsing, graph handles, or view visibility |
| Renderer scene/view preparation | `PreparedRenderScene` carries view-independent decal records; `RenderViewPreparation` owns the per-view tile plan; scene GPU capability owns persistent payload/dirty ranges and later receiver candidate spans | Shader blend policy duplicated in C++ or a second decal snapshot |
| GBuffer frame assembly | Skip-on-empty decision and one post-producer pass placement | Material ownership or backend barriers |
| Shared HLSL | Projection, sampling adapter, ordering assumptions, `SurfaceMaterial`, composition, GBuffer pack/decode adapters | Scene extraction or native API policy |
| Frame graph | Resource declarations, transitions, UAV barriers, transient tile buffers, pass marker and lifetime | Decal sorting or content semantics |
| RHI/backends | Existing texture SRV/UAV, structured-buffer, descriptor-table, and dispatch implementation | Decal concepts or a forward fallback |
| Showcase content | Sparkle-authored material textures and Modern Sponza placements | Claims that the fixture came from Intel |

No public RHI decal type is introduced. If implementation discovers a missing generic resource capability, add only that generic contract with D3D12/Vulkan parity and its own tests.

## Target Code Shape

Names may adjust to existing conventions during implementation, but ownership should remain recognizable:

```text
Engine/GameFramework
  Public/Scene/Decals/DecalDesc.h
  Private/World/ECS/Components/RenderingComponents.h
  Private/World/Extraction/...                 same object extraction path

Engine/Renderer
  Private/SceneData/DecalData.h
  Private/SceneData/Preparation/DecalPreparation.*
  Private/ShaderData/DecalShaderData.h
  Private/Passes/GBuffer/DeferredDecals.*         pass placement
  Private/Passes/GBuffer/DeferredDecalResolvePass.*

Engine/Assets/Shaders
  Material/SurfaceMaterial.hlsli                shared material value
  Decals/DecalProjection.hlsli
  Decals/DecalMaterialSampling.hlsli
  Decals/DecalComposite.hlsli
  Passes/GBuffer/Decals/DeferredDecalResolve.hlsl
```

Do not create separate `RasterDecal`, `RayDecal`, `DecalMaterial`, or `DecalTextureCache` directories/classes. The later ray lookup belongs beside existing ray scene preparation and adds no second copy of the shared files above.

The currently unconsumed `r.Material.BindingMode`/`MaterialBindingMode` path must be rechecked before implementation. If it remains unconsumed, delete it while renaming the capability around the scene material texture table needed by both ray materials and decals. Do not add a new decal binding-mode switch on top.

## Rejected Alternatives

| Alternative | Reason rejected now |
|---|---|
| Forward or scene-color decals | Lighting-dependent duplication, incomplete GBuffer semantics, and no GI/reflection material reuse. |
| Mesh decals as the primary feature | Different authoring/topology path and no clean arbitrary-hit material contract. |
| DBuffer textures | Extra bandwidth/storage and receiver-material integration without a baked-lighting requirement. |
| Hardware MRT alpha blending | Cannot preserve independently packed alpha/F0 or normalize normals with one blend factor; would also widen both RHI backends. |
| One pass or graph node per decal | Overlap races/barriers, command overhead, and graph clutter. |
| Full-screen loop over every decal | Cost scales with resolution times scene decal count. |
| Separate raster and ray decal materials | Duplicates asset identity, textures, sampling semantics, cache, and validation. |
| Reusing screen tiles for secondary rays | View-dependent data cannot cover arbitrary/off-screen reflection and GI hits. |
| Dedicated decal TLAS in the first slice | Adds AS/RHI/lifetime complexity before candidate-span evidence shows a need. |
| Dormant quality modes and CVars | More states to validate without a current content requirement. |
