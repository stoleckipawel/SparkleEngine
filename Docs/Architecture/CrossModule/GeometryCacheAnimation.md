# Geometry Cache Animation Pipeline

Status: target architecture; not implemented behavior
Date: 2026-08-17
Last source reconciliation: 2026-08-28 at committed `master` revision `20814381`; source and executable build configuration are unchanged from implementation revision `99af6d5b`
Scope: Alembic-authored baked mesh animation, native cooking and streaming, playback, shared raster/ray-tracing deformation, and D3D12/Vulkan parity

## Decision

Sparkle should support baked vertex animation as a first-class geometry-cache capability:

1. Alembic is a source format used by host tools only. The runtime never loads Alembic or USD.
2. The source importer normalizes axes, handedness, units, transforms, time samples, topology, attributes, face sets, and bounds into one `ImportedGeometryCache` contract.
3. The cooker publishes one range-readable native geometry-cache asset with immutable topology and a chunk directory followed by compressed sample data.
4. GameFramework owns instance playback state and publishes requested current and previous times through the existing render-scene identity and extraction lifecycle.
5. The Renderer owns chunk residency, decoding, GPU upload, deformation materialization, temporal history, and bounds used for rendering.
6. One renderer-owned `DeformedGeometry` product is generated before both ray-scene construction and GBuffer rasterization. Raster vertex fetch, ray-tracing BLAS geometry, and ray-hit material reconstruction consume that same product.
7. The first supported profile requires constant topology and stable vertex identity per track. Changing topology is rejected during cooking until a named workload justifies a separate accepted phase.

This is a target design, not proof that geometry caches are implemented. Code, cooked schemas, build configuration, tests, captures, and measurements remain the authority for implemented behavior.

This document owns geometry-cache source semantics, cooked data, playback-to-render extraction, streaming, shared deformation, and ray-tracing integration. The [Geometry Cache Animation Delivery Plan](../../Plans/CrossModule/GeometryCacheAnimation.md) owns phase order. The [Geometry Cache Animation Acceptance Contract](../../Acceptance/CrossModule/GeometryCacheAnimation.md) owns feature completion, while [Graphics Workloads](../../Acceptance/GraphicsWorkloads.md) owns map-wide evidence status. [World Coordinate, Units, and Transform Contract](../Decisions/WorldCoordinateAndUnits.md) owns canonical spatial conventions. [Renderer and RHI Architecture Boundary](../Decisions/RendererRhiBoundary.md) owns the feature/backend boundary.

## Outcome

The completed path supports one or more baked mesh tracks with:

- deterministic sample times, duration, looping, play rate, start offset, pause, and seek;
- immutable indices, UV topology, material sections, and stable vertex identity;
- static or sampled positions, normals, and tangents as declared by the cooked track;
- per-sample visibility and bounds when authored;
- current and previous deformed vertices for correct motion vectors and temporal consumers;
- asynchronous, bounded chunk streaming instead of full-cache startup retention;
- the same visible deformation in rasterized GBuffer, ray-traced primary visibility, reflections, GI, and path/reference lighting that use the scene TLAS.

It deliberately does not turn geometry caches into skeletal clips, morph-target banks, editable simulation, or a general scene-description format. The first delivery does not support changing topology, hair/curve/point primitives, particle caches, subdivision surfaces, cloth collision, retargeting, USD composition, or runtime Alembic parsing.

## Implementation Snapshot

The dated source inventory, absent capability, existing extension seams, and Modern Sponza Knight source facts live in the [Geometry Cache Animation Capability Snapshot](GeometryCacheAnimationCapability.md). Refresh that snapshot before beginning a delivery phase; this architecture does not claim the path is implemented.

## Requirements And Boundaries

### Required

- Host-tools-only Alembic integration with no runtime Alembic or USD dependency.
- One-time normalization to Sparkle's left-handed, `+Y` up, `+Z` forward, metre-based contract.
- Explicit source settings when an Alembic archive does not carry authoritative units or axis semantics. These are generic import settings, not per-scene runtime transforms.
- Constant topology, stable vertex identity, triangle indices, and stable material sections for the first supported profile.
- Named face-set material slots with explicit ordinary-material bindings. A missing or ambiguous binding fails cooking; it does not silently reuse the same texture in every semantic channel.
- Deterministic time sampling, chunk layout, cooked bytes, and output asset identity.
- Bounded async read/decode/upload residency and GPU-safe retirement through existing submission tokens.
- Independently sampled current and previous times, including loop boundaries, pause, seek, and discontinuity reset.
- One materialized current/previous geometry result shared by raster and ray-tracing consumers.
- D3D12 and Vulkan parity for raster delivery, resource states, GPU deformation, and later dynamic BLAS updates.
- Zero geometry-cache content means no cache worker requests, no chunk buffers, no deformation dispatch, and no dynamic geometry-cache BLAS work.
- Source-located cook/readiness errors for invalid topology, samples, attributes, material bindings, transforms, or unsupported primitives.

### Not in the first delivery

- Changing vertex or index count between samples.
- Curves, points, particles, hair, subdivision, or volume data.
- Runtime simulation, collision generation, retargeting, or animation editing.
- USD import, scene composition, or a runtime interchange-format abstraction.
- Per-frame logs, a geometry-cache diagnostics panel, playback CVars, codec selectors, or quality modes.
- A static-FBX fallback when cache data is missing or corrupt.
- Automatic material matching by filename, texture-name substring, face-set order, or knight-specific names.

If a source is outside the first profile, the feature remains unsupported and the cook fails with the track, sample time, and violated invariant. Less supported content is preferable to a hidden approximation.

## Unreal Engine Reference And Local Choice

Unreal is useful here because it treats this exact content class as geometry cache rather than forcing it into skeletal animation.

| Unreal behavior | Sparkle decision |
|---|---|
| Epic's Alembic importer describes Geometry Cache import as vertex-varying animation with frame-like playback. Face sets drive material slots, and normals, motion vectors, and changing topology have explicit import consequences. | Use Alembic as the source route, preserve face sets as named slots, and freeze a smaller constant-topology first profile. Do not infer a skeleton from controller curves. |
| `UGeometryCache` is a runtime asset containing tracks and materials, separate from the Alembic importer plugin. | Separate the tools-only importer from a native cooked runtime asset. Do not link Alembic into Engine or Renderer targets. |
| `UGeometryCacheComponent` owns playback state, while the asset owns sampled data. | Give GameFramework a small playback component and immutable cache handle; keep bulk samples out of ECS storage. |
| `UGeometryCacheTrackStreamable` and `FStreamedGeometryCacheChunk` use time-addressable streamable tracks whose metadata can remain resident while bulk chunks are unloaded. | Put a resident track/sample/chunk directory at the front of the cooked asset and range-read bounded chunks through the existing task and residency owners. |
| `FGeometryCacheMeshData` carries positions, tangent bases, UVs, colors, indices, motion vectors, batches, and bounds. | Define one canonical imported/cooked sample schema, classify attributes as static or sampled, preserve batches and bounds, and omit unsupported attribute classes explicitly. |
| Unreal exposes Geometry Cache as supported ray-tracing geometry and warns that deforming meshes pay dynamic BLAS build cost. | Feed the same materialized vertex buffer to raster and BLAS, then measure continuous BLAS update cost and memory before accepting ray parity. |

Primary Epic sources:

- [Alembic File Importer](https://dev.epicgames.com/documentation/unreal-engine/alembic-file-importer-in-unreal-engine?lang=en-US)
- [`UGeometryCache`](https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/GeometryCache/UGeometryCache?lang=en-US) and [`UGeometryCacheComponent`](https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/GeometryCache/UGeometryCacheComponent?lang=en-US)
- [`UGeometryCacheTrackStreamable`](https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/GeometryCache/UGeometryCacheTrackStreamable?lang=en-US), [`FStreamedGeometryCacheChunk`](https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/GeometryCache/FStreamedGeometryCacheChunk), and [`FGeometryCacheMeshData`](https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/GeometryCache/FGeometryCacheMeshData?lang=en-US)
- [GeometryCache module API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/GeometryCache) and [`FGeomCacheVertexFactory`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/GeometryCache/FGeomCacheVertexFactory)
- [Hardware Ray Tracing](https://dev.epicgames.com/documentation/unreal-engine/hardware-ray-tracing-in-unreal-engine) and [Ray Tracing Performance Guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/ray-tracing-performance-guide-in-unreal-engine)

Sparkle should adopt the separation and data-flow lessons, not copy Unreal's UObject, component, plugin, console-variable, or renderer structure.

## One Source And Cooked Contract

### Authored source asset

An Alembic archive does not reliably contain a portable PBR material definition or a universal unit convention. The first implementation therefore begins with a small project-owned geometry-cache source description consumed only by the cooker. A level references that source asset through the existing `[SceneAssets]` list.

Conceptually it owns:

```text
GeometryCacheSource
  AlembicPath
  SourceCoordinateOverride   optional, required when archive metadata is insufficient
  MaterialLibrarySource     ordinary glTF/FBX material source when needed
  MaterialSlotBindings      exact face-set name -> material name mappings
  DefaultPlayback           play rate, start offset, loop, initially active
```

The exact text syntax and extension are frozen in [Phase 0](../../Plans/CrossModule/GeometryCacheAnimation.md#phase-0---freeze-source-authoring-and-evidence-contracts) after the Modern Sponza Alembic face sets and metadata are inventoried. That decision must reuse the repository's existing level/discovery path and must not create a general asset database.

Every used face set has exactly one explicit material binding. The material library is imported through the existing material translation and cooker path; the geometry-cache importer does not grow a second PBR model. Empty slots are allowed only when the source description explicitly selects the ordinary default material. Missing textures retain the existing semantic checker/default behavior, but a missing slot mapping is an authoring error, not a residency fallback.

Playback defaults create ordinary instance state. They are not embedded into the immutable sampled-data asset, so future authored instances can use different time offsets or rates without duplicating cache bytes.

### Imported representation

Add a focused `ImportedGeometryCache` alongside, not inside, `ImportedMeshDeformation`:

```text
ImportedGeometryCache
  DurationSeconds
  Tracks[]

ImportedGeometryCacheTrack
  Name
  SampleTimes[]
  StaticTopology        indices, UVs, stable material sections
  AttributeModes       static or sampled position/normal/tangent
  Samples[]             changed attributes, visibility, local bounds
```

The importer validates finite monotonic sample times, triangle indices, vertex/attribute counts, stable topology and material sections, finite transforms, and bounds. It converts space and units once before publishing the imported value. It must retain source track/sample identity for diagnostics.

Alembic vertex-, varying-, and face-varying attributes are expanded through one stable render-vertex map derived from point and corner identity. If normals or tangents are absent, the cooker uses the existing deterministic mesh normal/MikkTSpace policy for every sample without changing that map. It records generated attributes in cook evidence; incomplete or inconsistent attribute streams do not silently reuse another sample's data.

The importer does not create one mesh per sample, one morph target per frame, or one entity per vertex. Source-format objects do not escape `Tools/Import`.

### Native cooked asset

Publish one native file per geometry-cache asset, for example `.sgca`, with the metadata needed for range reads at the front:

```text
CookedGeometryCacheHeader
  magic, schema identity, asset id, duration, track count, material slot count

TrackDirectory[]
  topology range, vertex/index counts, attribute modes, sample span, chunk span

SampleDirectory[]
  time, chunk id, offset within chunk, visibility, bounds

ChunkDirectory[]
  file offset, compressed bytes, decoded bytes, first/last sample, checksum

Payload
  static topology and material sections
  independently decodable sample chunks
```

The file is published atomically with the scene generation. Header, directory, offsets, sizes, integer conversions, checksums, and decompression limits are validated before allocation or publication.

Static UVs, indices, and material sections are stored once. Each section names one ordinary material slot and an index range; all sections of a track reference the same sampled vertex range. Sampled attributes are chunked by time. Chunk boundaries are independently decodable so a seek does not require replay from frame zero. Bounds are stored per sample or conservatively per chunk and interpolated conservatively.

[Phase 2](../../Plans/CrossModule/GeometryCacheAnimation.md#phase-2---native-cook-codec-and-range-reader) selects one production codec and chunk duration from deterministic Knight and synthetic-fixture measurements. The file records the one current schema and codec identity so stale output is rejected and regenerated; it does not dispatch to legacy readers or codecs. Uncompressed sample data may exist as a test oracle only; it is not a runtime fallback and is deleted from product code before the phase exits.

## Playback, Streaming, And Failure Semantics

### GameFramework playback

Add `GeometryCachePlaybackState` as a distinct ECS component with cache asset identity, local time, play rate, start offset, loop, playing state, and a discontinuity generation. A focused system advances time in the existing Animation phase and publishes one range per visible geometry-cache object:

```text
RenderGeometryCacheSampleRequest
  Object
  GeometryCache
  CurrentTimeSeconds
  PreviousTimeSeconds
  DiscontinuityGeneration
```

Previous time is evaluated independently. It is not always the previous displayed sample: pause produces equal times, loop evaluates on both sides of the wrap, and seek increments the discontinuity generation so motion history resets.

Heavy sample arrays, chunk tasks, decoded bytes, and GPU buffers never live in ECS components or per-frame extraction packets.

### Renderer residency

The structural render publication carries a geometry-cache handle and range-readable cooked path/metadata in the existing resource-table generation. The Renderer owns one cache keyed by asset generation and chunk id:

```text
requested time
  -> sample pair + interpolation factor
  -> required chunks
  -> BlockingIo range read
  -> background bounded decode
  -> render-thread upload
  -> resident sample views
```

The cache reuses `AssetResidency` states and budgets, `SparkleTasks` lanes, scene-generation cancellation, and submission-token retirement. It has one bounded look-ahead policy selected by measurement, not a new CVar. Requests publish immutable results only if their asset generation is still wanted.

Scene activation waits until the first required sample pair is renderable. During steady playback, a missed future chunk retains the last complete deformed product and resets motion when the new pair becomes available. It never mixes samples from different times, draws partially decoded vertices, substitutes the static FBX, or advances a corrupt range. The state transition is available to existing diagnostics/capture paths without per-frame log spam.

### Bounds and visibility

World bounds use the current sample pair's conservative interpolated local bounds transformed by the object transform. A hidden sample suppresses raster work and TLAS inclusion. A discontinuous seek publishes new bounds with the same generation as the new sample request.

No bind-pose or first-frame bounds may stand in for an animated track. Incorrect bounds would create camera-culling and ray-tracing omissions even when the vertices are correct.

## One Deformed Geometry Product

The shared boundary is materialized vertex data, not duplicated interpolation code:

```text
resident sample pair + interpolation factor
                    |
                    v
       GeometryCacheDeformation compute
                    |
                    v
DeformedGeometry { current/previous position, normal, tangent }
             |                    |                    |
             v                    v                    v
       raster GBuffer       dynamic BLAS        ray-hit attributes
```

The renderer allocates a stable range for each active geometry-cache track in a deformed-vertex pool. Position is first and the stride is valid as a generic ray-tracing triangle vertex buffer. Static indices, UVs, tangent sign, and material sections remain in the immutable topology mesh. The deformed range contains only attributes that vary per sample plus current/previous positions needed by temporal consumers.

`RenderDeformationPreparation` resolves the sample request, history/discontinuity state, and target range. One frame-graph compute pass interpolates and normalizes attributes into the range before any consumer. The frame graph declares the resource as unordered-access output followed by vertex/shader read and, when enabled, acceleration-structure build use.

The selected frame order becomes:

```text
Create scene resources
GeometryCacheDeformation                 skipped when empty
RayTracingSceneBuild                     reads current deformed positions
Rasterized or Raytraced GBuffer           reads shared current/previous attributes
Lighting / GI / reflections
```

Raster GBuffer fetches cache attributes by the draw's deformed range and keeps the existing base mesh path for static/skeletal draws. The ray hit path fetches the same current normal/tangent/position data and does not re-interpolate samples. Motion vectors compare independently sampled previous local positions transformed by the existing previous world matrix.

This design intentionally does not migrate skeletal deformation in the geometry-cache changelist. It establishes a reusable deformed-geometry product; a later measured cleanup may move skin/morph materialization behind the same product and delete the current CPU BLAS reconstruction, but geometry-cache acceptance must not be inflated into an unbounded animation rewrite.

## Ray-Tracing Integration

Constant topology allows reusable BLAS allocations for geometry-cache sections. The current RHI accepts one triangle geometry and one opacity classification per BLAS, so the first path uses one BLAS/TLAS instance per stable material section while all sections share the track's deformed vertex range. It does not add multi-geometry BLAS support unless Phase 5 measurements prove that the generic capability reduces total cost and complexity. The current RHI also exposes BLAS build but not update/refit. The ray phase therefore adds only the missing generic bottom-level build mode and flags, with matching D3D12 and Vulkan behavior:

- initial build allows update;
- continuous playback updates/refits from the current deformed vertex range;
- a topology identity change, asset generation change, or time discontinuity performs a full rebuild;
- scratch allocation covers the required build/update maximum;
- submission-token lifetime protects deformed vertices, indices, scratch, and acceleration structures.

The ray scene uses the same material, instance transform, visibility, and hit-record path as other triangle geometry. Reflections, GI, and path/reference lighting require no geometry-cache-specific shading effect: once the shared current attributes and BLAS are in the ordinary TLAS, those consumers see the deformation through their existing material-hit contract.

Record full-build versus update time, BLAS memory, scratch high-water, and total ray-scene build cost. If continuous update does not meet the accepted Knight budget on either backend, ray geometry-cache support remains blocked; do not silently omit the object from reflections/GI or substitute a static BLAS.

## Ownership

| Owner | Owns | Must not own |
|---|---|---|
| Project source description | Alembic path, source convention override, explicit face-set material bindings, instance playback defaults | Cooked offsets, GPU policy, knight-specific runtime scale |
| `Tools/Import/SourceImporters` | Alembic parsing, validation, canonical conversion, imported cache tracks, reuse of existing material import | Runtime playback, streaming threads, GPU resources |
| Scene/geometry-cache cooker | Cooked identity and current schema, static topology, chunking, codec, bounds, checksums, atomic publication, manifest references | Runtime cache policy or renderer handles |
| GameFramework asset loading | Header/directory validation, immutable asset handle and metadata, scene-generation publication | Full bulk-cache retention, decompression, GPU allocation |
| GameFramework ECS/systems | Playback state/time/discontinuity and ordinary entity lifetime | Chunk bytes, sample arrays, frame-graph work |
| Existing extraction | Structural cache handle and compact per-frame sample requests keyed by `RenderObjectId` | A second render-scene snapshot or cache thread pool |
| Renderer deformation preparation | Sample resolution, deformed ranges, current/previous history, bounds, skip-on-empty decision | Alembic parsing or gameplay playback policy |
| Renderer geometry-cache residency | Range reads, decode/upload state, budgets, look-ahead, cancellation, GPU-safe retirement | ECS ownership or per-feature task runtime |
| Frame graph | Compute ordering, external/transient resource declarations, barriers, and pass lifetime | Sample timing or cache eviction policy |
| Raster and ray consumers | Read the shared deformed range through their existing draw/hit contracts | Separate sample interpolation or cache decoding |
| RHI/backends | Generic buffer usage and bottom-level build/update operations | Geometry-cache types, sample clocks, codecs |
| Showcase | Source description, explicit material mappings, fixed cameras, evidence | Claims that static FBX proves animation |

## Target Code Shape

Names may adjust to repository conventions during implementation, but these ownership boundaries should remain visible:

```text
Tools/Import/SourceImporters
  Private/Alembic/...                         tools-only parser/translation
  Public/Types/ImportedGeometryCache.h

Tools/Cooking/GeometryCacheCooker
  ...                                         validation, chunking, codec, writer

Engine/GameFramework
  Public/Assets/Cooked/CookedGeometryCacheAsset.h
  Public/Rendering/RenderAssetHandles.h        cache handle
  Public/Rendering/RenderResourceTables.h      structural cache table
  Public/Rendering/RenderSceneDynamicData.h    compact sample requests
  Private/World/ECS/Components/...             playback state
  Private/Assets/Loaders/...                   metadata/directory only

Engine/Renderer
  Private/GeometryCaches/GeometryCacheResidency.*
  Private/GeometryCaches/GeometryCacheDeformedPool.*
  Private/Scene/Preparation/...                 target deformation preparation owner
  Private/Frame/Geometry/...                    one pre-consumer compute pass
  Private/RayTracing/Acceleration/...           shared range BLAS update

Engine/Assets/Shaders
  Geometry/GeometryCacheDeformation.hlsli
  Passes/Geometry/GeometryCacheDeformation.hlsl
```

Do not add `KnightImporter`, `ModernSponzaAnimation`, a geometry-cache renderer, a second animation graph, a second residency state machine, or raster/ray cache variants. If the target shape discovers a missing generic RHI or frame-graph buffer capability, add only that capability with backend parity and architecture-boundary coverage.

## Rejected Alternatives

| Alternative | Reason rejected |
|---|---|
| Treat controller channels as skeletal animation | The published FBX has no deform skeleton or skin weights; name remapping cannot create missing bindings. |
| Bake the first pose or a hand-selected pose | Proves static geometry only and hides the missing animation feature. |
| Convert every sample to a morph target | Duplicates data, abuses morph semantics, scales poorly, and still leaves streaming/RT unresolved. |
| Add a knight-only scale or transform | Source normalization belongs in the importer and must serve every FBX/Alembic asset. |
| Load USD at runtime | Expands into scene composition and runtime interchange dependencies when the archive already contains an Alembic cache. |
| Load Alembic at runtime | Couples runtime behavior and startup to a DCC interchange library and prevents controlled native cooking and streaming. |
| Fully preload the cache | Makes startup and memory scale with all samples; the published archive is too large to justify this as a production path. |
| Separate raster and ray interpolation | Risks different sample times, normals, and motion, and duplicates validation. |
| CPU-deform for ray tracing while shaders deform raster | Repeats the current skeletal split and prevents one authoritative deformed result. |
| Static BLAS in ray modes | Produces visibly wrong reflections, shadows, and GI while pretending the feature is supported. |
| Silent first-frame/static-FBX fallback | Masks missing/corrupt data and invalidates animation evidence. |
| Changing-topology support in the first slice | Adds sample-varying indices, hit-data remapping, BLAS rebuild policy, and material-batch churn before a required workload demonstrates the need. |
| New task pool, render scene, or residency state machine | Duplicates existing owners and complicates cancellation, budgets, and lifetime. |
| Codec/playback/debug CVars from day one | Creates unsupported combinations and test burden without an accepted content requirement. |
