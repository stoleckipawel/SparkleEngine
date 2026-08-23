# Deferred GBuffer Decal Pipeline

Status: target architecture and staged implementation plan; not implemented behavior
Date: 2026-08-17
Scope: projected material decals on opaque and alpha-tested GBuffer receivers, raster and ray-traced primary visibility, later arbitrary ray hits used by GI and reflections, D3D12/Vulkan parity, authoring, ownership, validation, and evidence

## Decision

Sparkle should implement decals as material overlays, never as a forward-lit color effect:

1. Rasterized and ray-traced primary visibility continue to produce the same base GBuffer.
2. One `DeferredDecalResolve` compute pass runs after either producer and before depth-derived and lighting consumers.
3. That pass reads the receiver depth and GBuffer, evaluates the visible decals for each covered pixel, and writes the composed material values back to the existing GBuffer.
4. Later GI and reflection rays apply the same projection, sampling, ordering, and material-composition functions at an arbitrary world-space hit. Only candidate lookup differs.
5. Decals reuse ordinary materials, the existing scene material texture table, `RenderObjectId`, frame-graph lifetime, and persistent GPU-scene update patterns. There is no decal material cache, forward fallback, DBuffer, second TLAS, or parallel ray material system.

This is a target design. Code, cooked schemas, build configuration, and executable tests remain the authority for what exists today.

This document owns decal semantics, data flow, pass placement, shared raster/ray composition, and delivery order. [Shader Authoring and Cooked Shader Architecture](Shaders/ShaderAuthoringAndCookedPrograms.md) owns shader identity/publication and the single implementation sequence for native RT pipeline/SBT/RHI delivery. The [Ray-Tracing Pipeline and Dual-Execution Target Architecture](Shaders/RayTracingPipelineImplementationPlan.md) owns the enduring inline-versus-pipeline and SBT semantics; adding decals must not create a third execution contract.

## Outcome

The completed path supports box-projected, softly blended decals that alter base color, world normal, metallic/roughness/AO/F0, emissive, and subsurface material values before lighting. The same authored decal can therefore appear:

- on a rasterized GBuffer receiver;
- on a primary surface written by the inline ray-traced GBuffer;
- in a later ray-traced reflection;
- in indirect-light material evaluation and bounce response.

It deliberately does not implement forward decals, mesh decals, transparent-surface decals, displacement, decal-cast geometry shadows, or emissive-decal light sampling. Those are separate capabilities and must not be implied by this plan.

## Current Repository Facts

The design extends the existing owner instead of adding a second renderer path:

- [`Passes/GBuffer/GBuffer.cpp`](../../Engine/Renderer/Private/Passes/GBuffer/GBuffer.cpp) creates one `GBufferRenderTargets` set and selects either `AddRasterizedGBufferPass` or `AddRaytracedGBufferPass`. Both branches meet before sky motion vectors and device-depth linearization.
- [`GBufferFormats.h`](../../Engine/Renderer/Private/Passes/GBuffer/GBufferFormats.h) defines the shared BaseColor, Normal, Material, Emissive, Subsurface, DeviceZ, and MotionVector products.
- [`GBufferPS.hlsl`](../../Engine/Assets/Shaders/Passes/Deferred/GBufferPS.hlsl) and [`RaytracedGBuffer.hlsl`](../../Engine/Assets/Shaders/Passes/RayTracing/RaytracedGBuffer.hlsl) already share [`GBufferPacking.hlsli`](../../Engine/Assets/Shaders/Passes/Deferred/GBufferPacking.hlsli).
- [`MaterialCache.cpp`](../../Engine/Renderer/Private/Scene/Materials/MaterialCache.cpp) resolves semantic defaults, per-material raster tables, and one scene-wide material texture table beneath the persistent render-scene authority. The latter must remain a scene-material capability, not be described as ray-tracing-only.
- [`RayTracingMaterialHit.hlsli`](../../Engine/Assets/Shaders/RayTracing/RayTracingMaterialHit.hlsli) is the central base-material reconstruction path for arbitrary ray hits. [`PathLighting.hlsli`](../../Engine/Assets/Shaders/RayTracing/PathLighting.hlsli) is one current secondary-hit consumer.
- The frame graph already derives unordered-access allocation and barriers from declared use. Raster depth is shader-readable on both backends. No new public RHI operation is required by the selected primary path.
- Graphics pipeline blending is currently fixed off in both backends. Adding blend state would not solve the semantic problem: GBuffer alpha components contain independent material data, including receiver alpha and dielectric F0, so one hardware source-alpha blend cannot express the required per-field preservation and normalized-normal composition.
- No decal component, cooked decal record, render-scene decal table, or decal shader exists today.

The exact code must be re-inspected at the start of each implementation phase because this document describes a target over a changing repository.

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

### Not in the first delivery

- Forward or scene-color decal fallback.
- Decals on alpha-blended/translucent surfaces, because those surfaces do not own a complete GBuffer receiver record.
- Mesh decals or topology-following projection.
- Animated projectors, skinned receiver candidate updates, or temporally correct moving-decal motion vectors.
- Per-channel blend modes, stain/modulate modes, height blending, parallax, or displacement.
- A dedicated decal acceleration structure, procedural AABB BLAS, or second TLAS.
- A decal editor panel, decal-specific material graph, runtime feature CVar, or per-frame log stream.

The first content is static by contract. Dynamic decals must not be advertised until motion, candidate invalidation, and temporal history behavior have their own accepted phase.

## Reference Findings And Local Choice

| Reference finding | Sparkle decision |
|---|---|
| Epic documents projected decal boxes, ordered overlap, receiver response, and GBuffer application after the Base Pass and before lighting. Its DBuffer path exists partly to interact correctly with baked lighting and adds receiver-material work and extra buffers. | Keep projected volumes, ordering, receiver opt-out, and pre-lighting material composition. Do not add a DBuffer while Sparkle has no baked-lighting requirement that justifies its storage and material-path cost. |
| Frostbite's classic deferred method reconstructs a world/local position from depth inside a convex volume, samples the decal, and blends GBuffer data. The presentation also identifies fixed-function alpha limitations when GBuffer alpha stores unrelated values and notes derivative/LOD hazards. | Use the proven depth-reconstructed projection model, but use programmable read/modify/write composition and explicit gradient-based texture sampling. |
| The i3D ray-tracing decal work shows that view-frustum grids do not serve arbitrary reflection hits and that a ray-tracing acceleration structure can enumerate decals anywhere, at higher cost than classic deferred decals. | Keep the screen-space primary path. For arbitrary hits, begin with receiver candidate spans built from existing world bounds. Consider a dedicated AABB AS only if measured candidate counts make that simpler or faster overall. |
| Ray Tracing Gems II surveys triangle and procedural decal approaches, single and multiple overlaps, and their costs. | Treat mesh/procedural AS decals as measured alternatives for the later ray phase, not as a prerequisite or a second implementation now. |
| Intel's Modern Sponza is a high-resolution PBR workload with 4K textures and separately listed add-ons; the published list does not include a decal package. | Add a small Sparkle-authored decal fixture and label it as such. Do not attribute those decals to Intel's content. |

Sources:

- Epic Games, [Decal Materials](https://dev.epicgames.com/documentation/en-us/unreal-engine/decal-materials-in-unreal-engine) and [Decal Actors](https://dev.epicgames.com/documentation/unreal-engine/decal-actors-in-unreal-engine?lang=en-US)
- Johan Andersson and Daniel Kihl, [Destruction in Frostbite](https://advances.realtimerendering.com/s2010/Kihl-Destruction%20in%20Frostbite%28SIGGRAPH%202010%20Advanced%20RealTime%20Rendering%20Course%29.pdf), SIGGRAPH 2010 course material
- Sidney Hansen and Christoph Peters, [Rendering Decals and Many Lights with Ray Tracing Acceleration Structures](https://i3dsymposium.org/2021/posters/hansen2021_rendering_decals_and_many_lights_paper.pdf), i3D 2021
- Wessam Bahnassi, [Ray Tracing Decals](https://link.springer.com/chapter/10.1007/978-1-4842-7185-8_27), Ray Tracing Gems II
- Intel, [GPU Research Samples](https://www.intel.com/content/www/us/en/developer/topic-technology/graphics-research/samples.html)

## One Decal Contract

### Authored value

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

### Runtime identity and lifetime

Decals use the existing `RenderObjectId`, world transform, visibility, create/update/destroy sequence, and render-scene generation. Evolve `RenderObjectStaticData` into an explicit mesh-or-decal payload rather than creating a second object delta protocol. An entity that attempts to publish both payload kinds fails validation until a real combined-object use case exists.

The Renderer owns:

- one scene-owned decal table projected into frame-slot `PreparedRenderScene` data;
- one persistent structured GPU `DecalData` buffer updated through the existing dirty-range pattern;
- one per-view primary tile plan;
- later, one flat secondary-ray candidate index buffer and a span in each ray hit instance.

The GPU decal record contains only stable indices and POD: world-to-decal transform, projector basis or the data needed to derive it, material slot, opacity/fades, channel mask, sort order, and object-id tie-break value. It contains no descriptors, native handles, owning pointers, or duplicated material constants.

### Deterministic order

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
        `-- RaytracedGBuffer inline-query CS +
                                             |
                              DeferredDecalResolve CS
                                             |
                              Sky motion / depth linearization
                                             |
                              Lighting / GI / reflections / post
```

The pass modifies BaseColor, Normal, Material, Emissive, and Subsurface as UAVs. It samples DeviceZ and leaves DeviceZ and MotionVector unchanged. The frame graph owns render-target/depth-to-SRV/UAV transitions and the transition back to lighting reads.

This placement is producer-independent. Phase 2 accepts raster behavior first; Phase 3 then proves the already-shared pass against the ray-traced primary producer. Do not add a temporary duplicate ray decal pass or a mode-specific shader.

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

The tile size is a compile-time implementation constant selected during Phase 2 measurement, not a user CVar. If CPU binning becomes measurable, replace it with one demonstrated better owner and delete the old builder; do not retain two selectable planners.

### Per-pixel sequence

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

A screen tile cannot answer which decals overlap a reflection hit outside the camera frustum. The later ray phase therefore shares decal data and evaluation but uses a view-independent candidate lookup.

### Initial lookup

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

Secondary texture filtering must receive an explicit surface footprint. If the ray path still uses the current fixed LOD-zero material sampling when Phase 4 begins, introduce one reusable ray-cone/differential or conservative explicit-LOD contract for ordinary ray-hit materials and decals together. Do not add a decal-only filtering model, and do not accept a reflection path that aliases because every decal sample silently uses mip zero.

The first implementation may rebuild the flat candidate product when decal topology/transforms or receiver bounds change. That is correct for the static initial contract. Candidate count per receiver, total links, rebuild CPU time, and buffer high-water are evidence fields for the phase; they are not a permanent UI product.

### Integration rule

Keep base hit reconstruction separate from decal application:

```text
ReconstructRayTracingHitSurface
ApplyDecalsAtRayHit
ShadeRayTracingHitSurface
```

Call the middle operation for GI, reflection, and path-lighting material hits. Do not call it for:

- primary `RaytracedGBuffer`, because the shared screen resolve owns primary decals;
- shadow/visibility rays, because this decal contract changes material values, not geometry or alpha-test opacity;
- misses or invalid surfaces.

Phase 4 must inventory every `ReconstructRayTracingHitSurface` consumer and classify it explicitly as primary material, arbitrary shaded material, shadow/visibility, or unsupported. No effect gets a private decal implementation.

### Why no decal TLAS first

A procedural AABB acceleration structure can enumerate arbitrary overlaps and remains a valid measured alternative. It is not the initial choice because Sparkle currently owns triangle BLAS/TLAS products, while procedural decal geometry would add RHI geometry contracts, build inputs, hit-group/pipeline considerations, another lifetime product, and backend validation before proving that Modern Sponza needs them.

Adopt a dedicated decal AS only when receiver spans fail a recorded candidate-count or frame-time budget on the accepted workloads. If adopted, it replaces the span lookup while preserving the authored data and shared shader composition. It must not become a second decal system.

## Ownership

| Owner | Owns | Must not own |
|---|---|---|
| GameFramework scene/cooker | `Decal` component/schema, ordinary material reference, authored transform and parameters, source validation, cooked versioning | GPU descriptors, frame order, GBuffer formats |
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
  Passes/Deferred/DeferredDecalResolve.hlsl
```

Do not create separate `RasterDecal`, `RayDecal`, `DecalMaterial`, or `DecalTextureCache` directories/classes. The later ray lookup belongs beside existing ray scene preparation and adds no second copy of the shared files above.

The currently unconsumed `r.Material.BindingMode`/`MaterialBindingMode` path should be rechecked in Phase 1. If it remains unconsumed, delete it while renaming the capability around the scene material texture table needed by both ray materials and decals. Do not add a new decal binding-mode switch on top.

## Staged Delivery

Each phase is independently reviewable. A later phase starts only after the preceding exit gate is recorded. A phase that replaces a temporary path deletes it in the same change.

### Phase 0 - Freeze contracts and evidence

Deliver documentation and executable contract tests before runtime decal output:

- freeze projector axes, unit volume, UV orientation, coverage, channel groups, overlap order, receiver bit, and static-only scope;
- add CPU reference tests for box/facing/edge projection and every composition group;
- define D3D12/Vulkan image-comparison cameras and timing capture fields;
- record the implementation budget at the accepted resolution instead of inventing a post-hoc threshold;
- audit every GBuffer and ray-hit surface consumer and record migration/deletion obligations;
- define the Sparkle-owned Modern Sponza fixture and provenance.

Exit: contracts are reviewed, test data is deterministic, workload/cameras are named, and there is no runtime feature claim.

### Phase 1 - Authoring, scene ownership, and GPU data

- add the decal ECS/cooked schema using an ordinary material reference;
- extend the existing render-object lifecycle with a mesh-or-decal payload;
- add the receiver boolean to mesh instance data;
- prepare one renderer decal table and persistent dirty-range GPU buffer;
- make the existing scene material texture table available as a general renderer material resource on supported D3D12/Vulkan devices;
- remove the dead binding-mode concept if the Phase 0 audit confirms it has no consumer;
- fail invalid projector transforms, material references, channel masks, and table capacity at the owning publication boundary with source identity;
- add the repository-owned fixture assets and placements, but keep rendering disabled until Phase 2.

Exit: cook/load/extract/create/update/destroy round trips pass; zero decals publish no decal GPU allocation; no rendering or ray claim is made.

If a scene contains decals but the required scene material table is unsupported, readiness fails once with the missing capability and source scene identified. There is no silent omission and no forward fallback.

### Phase 2 - Rasterized deferred decals

- introduce `SurfaceMaterial` and shared projection/composition HLSL;
- preserve current material sampling bindings while deleting duplicated PBR field definitions that the new value actually replaces;
- write/read the receiver bit through the unused normal alpha channel;
- build the per-view active-tile plan with deterministic ordering;
- add one `DeferredDecalResolve` compute pass after the common GBuffer producer join;
- enable and accept the feature first with `GBufferMode::Rasterized` on D3D12 and Vulkan;
- use existing GBuffer visualization and a single pass marker for inspection.

Exit: raster image tests pass for every channel group, overlap, near-plane/camera-inside volumes, projection edges, jitter, receiver opt-out, and zero-decal identity. The chosen tile plan meets the recorded CPU/GPU/memory budget on the fixture and an overlap stress case.

### Phase 3 - Ray-traced primary GBuffer parity

- exercise the same post-producer pass with `GBufferMode::Raytraced`;
- reconcile any depth reconstruction or receiver-bit production difference at the producer adapters, not in a ray-specific decal shader;
- compare the raster and ray-traced primary results from the same camera, scene, material table, ordering, and output formats on both backends;
- keep `RaytracedGBuffer` base-hit reconstruction decal-free so primary decals are not applied twice.

Exit: paired primary images agree within the recorded format/tolerance budget, pass ordering is identical, and there is still exactly one primary decal pass.

### Phase 4 - Reflections and GI material hits

- build stable per-receiver candidate spans from existing receiver world bounds and decal boxes;
- append span fields to the existing ray hit instance payload and publish one candidate-index buffer;
- call the shared projection/sampling/composition functions at the central arbitrary-hit material seam;
- provide one ordinary-ray-material texture footprint/LOD contract and use it for decal samples too;
- classify and migrate all hit-surface consumers; shadow/visibility and primary-GBuffer exclusions stay explicit;
- validate reflected base color/normal/roughness, indirect bounce response, and emissive-at-hit behavior in reference/path and real-time ray modes that consume those material hits;
- record candidate links, candidates tested per shaded hit, rebuild time, buffer high-water, and shader time in the existing evidence route.

Exit: the same authored decal appears on the visible receiver and in an off-screen reflection, changes indirect material response where expected, and matches the CPU projection/composition reference. Candidate spans meet the recorded budget on Modern Sponza and the overlap stress case.

### Phase 5 - Acceptance and cleanup

- run the full raster/ray-primary/ray-secondary matrix on D3D12 and Vulkan;
- capture the fixed Modern Sponza cameras and one adversarial overlap/near-plane scene;
- run existing architecture, shader ABI, formatting, build, and workload checks selected by the touched standards;
- remove temporary guards, duplicate structs, abandoned planners, unused flags, and stale documentation discovered by the migration;
- update this document from target plan to implemented architecture only for behavior proven by code and evidence.

Exit: there is one authored contract, one runtime decal table, one primary resolve, one shared shader composition path, and one selected secondary lookup. Unavailable checks and unsupported surfaces remain stated rather than hidden by fallbacks.

## Modern Sponza Validation Fixture

Intel's source package remains external and unmodified. Add a small repository-owned decal fixture under Showcase ownership and place it from the Modern Sponza level composition. Keep it visually intentional and small enough for review:

| Placement | Contract exercised |
|---|---|
| Broad wall damp/grime patch | BaseColor + Material, large screen coverage, soft depth edge |
| Chipped plaster/crack | Normal + roughness, grazing-angle facing fade |
| Faded painted mark | BaseColor alpha coverage and stable close inspection |
| Floor wet patch | Normal + roughness under reflected lighting |
| Two-layer repair/mark stack | Stable overlap order and all producer parity |
| Volume crossing an opted-out receiver | Exact box test and `ReceivesDecals` behavior |

The fixture must state its own texture license/provenance. Illustrative values or captures are not Intel reference measurements. Modern Sponza remains a compatibility workload under [`BistroAndSanMiguelWorkloads.md`](../Engineering/BistroAndSanMiguelWorkloads.md), not a replacement for its larger acceptance gates.

## Validation Matrix

| Surface | Raster primary | Ray primary | Ray reflection/GI hit |
|---|---:|---:|---:|
| Opaque static mesh | Phase 2 | Phase 3 | Phase 4 |
| Alpha-tested static mesh | Phase 2 | Phase 3 | Phase 4 |
| Receiver opted out | Unchanged | Unchanged | Unchanged |
| Sky/miss | Unchanged | Unchanged | Unchanged |
| Alpha-blended/transparent | Unsupported | Unsupported | Unsupported |
| Skinned or moving receiver | Static-frame appearance only; temporal support deferred | Same | Candidate update support deferred |

Minimum automated coverage:

- projection inside/outside, orientation, UV, edge/facing fade, negative/degenerate transform rejection;
- composition masks, zero/one/intermediate coverage, normal normalization, receiver alpha preservation, and F0 preservation when the Material group is disabled;
- stable ordering with equal and unequal sort values;
- tile candidates against a brute-force CPU oracle, including near-plane and camera-inside cases;
- secondary candidate spans against brute-force OBB/receiver overlap;
- GBuffer pack/decode round trip for every affected field and receiver bit;
- zero-decals graph omission and unchanged image;
- D3D12/Vulkan shader ABI and image parity;
- fixed-camera raster versus ray-primary comparison;
- visible versus reflected/indirect appearance for the same decal.

Performance evidence records existing frame time and the one pass duration, CPU plan time, upload bytes, active tiles, tile candidate indices, secondary candidate links, and shaded-hit candidate count. These belong in the existing capture/evidence workflow. Do not add a decal dashboard, periodic logging, or a new diagnostics subsystem.

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

## Completion Definition

Deferred decals are complete only when all of the following are true:

- authors place one decal kind that references one ordinary material kind;
- zero content has zero runtime pass cost;
- raster and ray-traced primary visibility use the same post-GBuffer resolve;
- GI/reflection material hits use the same projection and composition functions;
- overlap order and receiver behavior match on D3D12 and Vulkan;
- Modern Sponza fixture images and performance evidence satisfy the recorded gates;
- no forward fallback, DBuffer, decal material cache, duplicate ray material path, or unowned diagnostic surface remains;
- the docs distinguish implemented behavior from future dynamic/transparent/mesh-decal work.
