# Geometry Cache Animation Pipeline

Status: target architecture and staged implementation plan; not implemented behavior
Date: 2026-08-17
Scope: Alembic-authored baked mesh animation, native cooking and streaming, playback, shared raster/ray-tracing deformation, D3D12/Vulkan parity, Modern Sponza Animated Knight acceptance, and evidence

## Decision

Sparkle should support baked vertex animation as a first-class geometry-cache capability:

1. Alembic is a source format used by host tools only. The runtime never loads Alembic or USD.
2. The source importer normalizes axes, handedness, units, transforms, time samples, topology, attributes, face sets, and bounds into one `ImportedGeometryCache` contract.
3. The cooker publishes one versioned, range-readable native geometry-cache asset with immutable topology and a chunk directory followed by compressed sample data.
4. GameFramework owns instance playback state and publishes requested current and previous times through the existing render-scene identity and extraction lifecycle.
5. The Renderer owns chunk residency, decoding, GPU upload, deformation materialization, temporal history, and bounds used for rendering.
6. One renderer-owned `DeformedGeometry` product is generated before both ray-scene construction and GBuffer rasterization. Raster vertex fetch, ray-tracing BLAS geometry, and ray-hit material reconstruction consume that same product.
7. The first supported profile requires constant topology and stable vertex identity per track. Changing topology is rejected during cooking until a named workload justifies a separate accepted phase.

This is a plan, not proof that geometry caches are implemented. Code, cooked schemas, build configuration, tests, captures, and measurements remain the authority for implemented behavior.

This document owns geometry-cache source semantics, cooked data, playback-to-render extraction, streaming, shared deformation, ray-tracing integration, and delivery order. [World Coordinate, Units, and Transform Contract](WorldCoordinateAndUnits.md) owns canonical spatial conventions. [Renderer and RHI Architecture Boundary](RendererRhiBoundary.md) owns the feature/backend boundary. The [Bistro and San Miguel Acceptance Workloads](../Engineering/BistroAndSanMiguelWorkloads.md) document owns map acceptance and evidence status.

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

## Current Repository Facts

The plan extends existing owners instead of adding a parallel animation or render scene:

- [`SourceSceneImporter.cpp`](../../Tools/Import/SourceImporters/Private/SourceSceneImporter.cpp) selects glTF or FBX from a hard-coded importer list. [`ImportedScene.h`](../../Tools/Import/SourceImporters/Public/Types/ImportedScene.h) carries meshes, instances, materials, skeletons, and clips but no geometry-cache tracks. No Alembic dependency exists in the current source dependency manifest.
- [`ImportedMeshDeformation.h`](../../Tools/Import/SourceImporters/Public/Types/ImportedMeshDeformation.h) contains skin influences and morph targets. Expanding one cache sample per frame into morph targets would misuse that contract and scale memory with `vertex count * sample count`.
- [`ImportedSceneCooker.cpp`](../../Tools/Cooking/AssetCooker/Private/Cooking/ImportedSceneCooker.cpp) is the scene-cook orchestrator. [`CookedSceneBuild.h`](../../Tools/Cooking/SceneCooker/Public/CookedSceneBuild.h) and [`CookedSceneGenerationWriter.cpp`](../../Tools/Cooking/SceneCooker/Private/CookedSceneGenerationWriter.cpp) already stage one atomic generation of meshes, materials, skeletons, animations, and manifests.
- [`SceneAssetFileReader.cpp`](../../Engine/GameFramework/Private/Assets/Loading/SceneAssetFileReader.cpp) currently reads complete mesh, material, skeleton, and animation products for scene activation. A geometry cache cannot join that full-file retention path; activation should read only its header and chunk directory.
- [`AnimationComponents.h`](../../Engine/GameFramework/Private/World/ECS/Components/AnimationComponents.h) and the animation systems own skeletal and morph playback/output. Geometry-cache playback needs its own small component because its output is sampled vertex data, not a pose or weight vector.
- [`RenderSceneDelta.h`](../../Engine/GameFramework/Public/Rendering/RenderSceneDelta.h) carries structural mesh/material state, while [`RenderSceneDynamicData.h`](../../Engine/GameFramework/Public/Rendering/RenderSceneDynamicData.h) carries per-frame transforms, joint matrices, and morph weights. Those are the existing structural and dynamic scene-publication seams to extend.
- The current `RenderDeformationPreparation` implementation is under `Renderer/Private/SceneData/Preparation`. The committed renderer migration moves scene deformation continuity beneath `RenderScene` and view-independent frame materialization beneath `Scene/Preparation/RenderScenePreparation`; geometry-cache preparation belongs in that target owner or a focused collaborator owned by it, never another scene-preparation graph.
- [`GBufferVS.hlsl`](../../Engine/Assets/Shaders/Passes/Deferred/GBufferVS.hlsl) currently evaluates morphing and skinning in the raster vertex shader. [`RayTracingMaterialHit.hlsli`](../../Engine/Assets/Shaders/RayTracing/RayTracingMaterialHit.hlsli) repeats their attribute reconstruction at ray hits.
- [`RayTracingBlasGeometryBuilder.cpp`](../../Engine/Renderer/Private/RayTracing/Acceleration/RayTracingBlasGeometryBuilder.cpp) currently reconstructs skinned positions on the CPU, and [`RayTracingBlasCache.cpp`](../../Engine/Renderer/Private/RayTracing/Acceleration/RayTracingBlasCache.cpp) uploads a replacement vertex buffer and rebuilds its BLAS. Geometry-cache delivery must not copy this CPU-versus-shader split.
- [`Frame.cpp`](../../Engine/Renderer/Private/Frame/Core/Frame.cpp) builds the ray-tracing scene before the GBuffer. Shared deformed geometry must therefore be ready before both consumers rather than inserted inside either one.
- [`AssetResidency`](../../Engine/Renderer/Private/Resources/Residency/AssetResidency.h) already owns read/decode/upload/resident/retirement state and byte budgets. [`SparkleTasks`](../../Engine/Tasks/Public/TaskTypes.h) already provides a bounded blocking-I/O lane. Geometry-cache streaming should reuse both instead of creating a cache-specific thread pool or second lifetime protocol.

The Modern Sponza Animated Knight archive is the motivating workload, not a reason for a scene-specific fix:

- `Knight_USD_002.fbx` contains renderable meshes and controller animation channels but no bones or skin weights. It can prove static FBX import and unit normalization; it cannot produce visible skeletal deformation.
- `Knight_Animation_Data_Only_002.fbx` also has no deform skeleton or skin binding.
- the Maya source contains skin clusters, bind-pose data, joints, and animation curves;
- `Exports/alembic/knight_ANIM_001.rnd.abc` and the published USD contain baked vertex deformation. The Alembic is the source intended for a geometry-cache path.

The knight remains unaccepted as an animated workload until the final phase of this plan passes. Loading the static FBX, baking one pose, remapping controller names, or scaling only the knight entity does not close that gate.

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

The exact text syntax and extension are frozen in Phase 0 after the Modern Sponza Alembic face sets and metadata are inventoried. That decision must reuse the repository's existing level/discovery path and must not create a general asset database.

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
  magic, version, asset id, duration, track count, material slot count

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

Phase 2 selects one production codec and chunk duration from deterministic Knight and synthetic-fixture measurements. The file reserves a versioned codec identifier for format evolution, but a shipping profile registers one codec, not a menu of dormant implementations. Uncompressed sample data may exist as a test oracle only; it is not a runtime fallback and is deleted from product code before the phase exits.

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
| Scene/geometry-cache cooker | Cooked identity/version, static topology, chunking, codec, bounds, checksums, atomic publication, manifest references | Runtime cache policy or renderer handles |
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

## Staged Delivery

Each phase is independently reviewable. A later phase starts only after the preceding exit gate is recorded. No phase may advertise animation from a frozen pose or a static fallback. Temporary reference code is deleted in the phase that replaces it.

### Phase 0 - Freeze source, authoring, and evidence contracts

- inventory every Alembic track, sample time, topology count, face set, material name, attribute scope, visibility sample, transform, bound, unit/axis cue, and archive time range for the Knight and a tiny licensed fixture;
- freeze the source-description syntax and extension, exact material binding rule, coordinate override rule, stable render-vertex expansion, generated normal/tangent policy, playback defaults, and source-root safety rules;
- freeze constant-topology support and source-located rejection messages for every excluded schema;
- define the imported and cooked schemas, versioning policy, asset identity, maximum counts/bytes, checksum policy, and range-read validation;
- select fixed raster/ray cameras, playback intervals, loop/seek cases, motion-vector inspection, profiler fields, and accepted memory/frame budgets before implementation;
- record the measured reference output for at least two Knight samples independently of Sparkle.

Exit: contracts and fixtures are reviewed, archive facts are recorded, budgets are named, and there is no runtime feature claim.

### Phase 1 - Tools-only Alembic import

- add one pinned Alembic dependency to the content-pipeline build only, with license and reproducible dependency configuration;
- extend the existing source-discovery/importer registry rather than adding an AssetCooker special case;
- parse the source description, Alembic archive, and optional ordinary material-library source;
- produce `ImportedGeometryCache` tracks in canonical Sparkle space/metres with exact material bindings;
- reject unstable topology, vertex identity, batches, unsupported primitives, invalid sample times, non-finite attributes, and unsafe dependent paths;
- add deterministic importer tests for coordinates, time, attributes, face sets, multi-track input, and negative cases.

Exit: the tiny fixture and Knight source import deterministically into the canonical model; no cooked/runtime support is claimed; Alembic symbols are absent from Engine, Renderer, and game binaries.

### Phase 2 - Native cook, codec, and range reader

- add the native geometry-cache asset, manifest reference, path helper, cooker output, and atomic generation staging;
- keep static topology/UV/material sections once and write independently decodable time chunks;
- measure candidate chunk durations and one production codec on the fixture and Knight, then retain one selected implementation;
- validate header/directory arithmetic, decompression bounds, checksums, truncated/corrupt data, deterministic output hashes, and source recook invalidation;
- make scene loading retain only validated metadata/directory state while bulk samples remain range-readable;
- publish ordinary material assets through the existing material cooker and prove every face-set mapping.

Exit: cook/read/seek/decode tests reproduce source samples within the frozen numeric tolerance; compressed size, peak cook memory, range-read bytes, and decode time meet the Phase 0 budgets; no runtime draw is claimed.

### Phase 3 - Playback, residency, and shared deformation foundation

- add geometry-cache asset/resource handles, scene-manifest records, instance construction, playback component, system ordering, and compact extraction requests;
- extend the existing render-object classification and lifecycle with geometry-cache topology plus a cache handle;
- add bounded look-ahead range requests through `SparkleTasks`, `AssetResidency`, scene cancellation, and submission-safe retirement;
- extend `RenderDeformationPreparation` with cache sample resolution and discontinuity history;
- allocate the shared deformed-geometry pool and add one frame-graph compute producer before ray-scene/GBuffer consumers;
- test pause, play rate, loop wrap, seek, unload/reload, cancellation, starvation, zero-content omission, and current/previous output on a compute-readback fixture.

Exit: deterministic GPU readback matches the CPU sample oracle on D3D12 and Vulkan; bounds/visibility and history resets are correct; memory remains bounded during long playback and random seeks; rendering is not yet accepted.

### Phase 4 - Rasterized GBuffer and temporal acceptance

- add the geometry-cache range to existing draw and GPU-scene payloads without changing static/skeletal behavior;
- make the GBuffer vertex path fetch shared current/previous positions, normals, and tangents while retaining static UV/index/material data;
- include current sample bounds in culling and batching without creating a geometry-cache draw list when the ordinary mesh batching contract can be extended;
- validate opaque and alpha-tested material sections, normal mapping, motion vectors, pause, loop, seek, camera motion, and object motion;
- compare fixed sample times against the independent source reference and capture D3D12/Vulkan image parity;
- measure streaming, decode, upload, deformation-dispatch, raster, frame-time, and CPU/GPU memory high-water.

Exit: the fixture and Knight visibly deform at correct scale, placement, material slots, and time in rasterized GBuffer on both backends; motion/discontinuity tests pass; no static or first-frame fallback is active.

### Phase 5 - Ray-traced GBuffer, reflections, and GI

- add generic RHI bottom-level build/update flags and modes with D3D12/Vulkan contract tests;
- bind each geometry-cache BLAS to the shared current deformed range and immutable topology indices;
- update continuous fixed-topology playback and rebuild on asset/discontinuity boundaries;
- make the central ray-hit attribute path fetch the same current deformed normal/tangent/position data used by raster;
- validate ray-traced primary GBuffer, off-screen reflection visibility, indirect/GI material response, path/reference lighting, shadows, loop, and seek;
- record BLAS full/update time, scratch/resident bytes, TLAS cost, ray-hit cost, and raster-versus-ray image comparisons.

Exit: raster and ray-primary surfaces agree within the recorded tolerance; the animated Knight appears correctly in reflections and GI on D3D12 and Vulkan; measured BLAS cost meets the Phase 0 budget. A backend that fails remains unsupported explicitly rather than receiving a static substitution.

### Phase 6 - Modern Sponza Knight acceptance and cleanup

- switch only `ModernSponzaKnight.level` from the static FBX source to the reviewed geometry-cache source description;
- preserve the general FBX unit-normalization fix for other FBX content; add no knight-only scale, transform, clip, or material condition;
- run the map acceptance sequence from [`BistroAndSanMiguelWorkloads.md`](../Engineering/BistroAndSanMiguelWorkloads.md), including raster/ray cameras, visible motion, loop stability, streaming pressure, unload/reload, and backend parity;
- remove temporary import dumps, reference decoders, duplicate structs, abandoned codec experiments, stale static-animation claims, and any unused compatibility branches;
- update this document from target plan to implemented architecture only for behavior proven by code and evidence.

Exit: Modern Sponza Animated Knight is accepted as geometry-cache animation with one tools source path, one cooked asset contract, one runtime playback state, one streamed residency owner, and one shared deformed-geometry product. Unavailable or failed checks remain recorded.

Only after this exit should the paused deferred-decal implementation proceed.

## Verification Matrix

| Contract | Automated evidence | Runtime/capture evidence |
|---|---|---|
| Source normalization | axis/unit/handedness/transform fixture and Knight metadata inventory | fixed source-reference sample overlay |
| Topology profile | stable topology positive fixture; vertex/index/batch drift rejection | no partial or frozen rendering for rejected content |
| Material slots | face-set mapping round trip; missing/duplicate mapping rejection | correct Knight material separation and semantic texture channels |
| Cooked format | deterministic hash; corrupt/truncated/overflow/checksum tests; random-access sample oracle | bounded startup bytes and seek latency |
| Playback | pause/rate/offset/loop/seek/discontinuity unit tests | visible loop and scrub stability |
| Residency | budget, cancellation, generation replacement, starvation, retirement tests | long playback and random-seek memory high-water |
| Shared deformation | CPU versus GPU current/previous readback | matching raster/ray geometry at fixed times |
| Raster | shader ABI, zero-content omission, bounds/culling tests | D3D12/Vulkan GBuffer, normals, materials, motion vectors |
| Ray tracing | generic BLAS update contract tests and resource lifetime tests | primary, shadow, reflection, GI/path visibility and cost |
| Cleanup | architecture-boundary check, build membership, `git diff --check`, no Alembic runtime linkage | no scene-name branches, fallback pose, or permanent diagnostic clutter |

The Knight evidence record includes source and cooked bytes, track/sample/vertex/triangle counts, cook time and peak memory, compression ratio, read/decode/upload latency, resident CPU/GPU bytes, deformation pass time, BLAS build/update time and memory, total frame time, and fixed-camera captures. Use existing profiler/capture mechanisms. Do not add a permanent geometry-cache dashboard or periodic log stream.

## Rejected Alternatives

| Alternative | Reason rejected |
|---|---|
| Treat controller channels as skeletal animation | The published FBX has no deform skeleton or skin weights; name remapping cannot create missing bindings. |
| Bake the first pose or a hand-selected pose | Proves static geometry only and hides the missing animation feature. |
| Convert every sample to a morph target | Duplicates data, abuses morph semantics, scales poorly, and still leaves streaming/RT unresolved. |
| Add a knight-only scale or transform | Source normalization belongs in the importer and must serve every FBX/Alembic asset. |
| Load USD at runtime | Expands into scene composition and runtime interchange dependencies when the archive already contains an Alembic cache. |
| Load Alembic at runtime | Couples runtime behavior and startup to a DCC interchange library and prevents controlled native versioning/streaming. |
| Fully preload the cache | Makes startup and memory scale with all samples; the published archive is too large to justify this as a production path. |
| Separate raster and ray interpolation | Risks different sample times, normals, and motion, and duplicates validation. |
| CPU-deform for ray tracing while shaders deform raster | Repeats the current skeletal split and prevents one authoritative deformed result. |
| Static BLAS in ray modes | Produces visibly wrong reflections, shadows, and GI while pretending the feature is supported. |
| Silent first-frame/static-FBX fallback | Masks missing/corrupt data and invalidates animation evidence. |
| Changing-topology support in the first slice | Adds sample-varying indices, hit-data remapping, BLAS rebuild policy, and material-batch churn before a required workload demonstrates the need. |
| New task pool, render scene, or residency state machine | Duplicates existing owners and complicates cancellation, budgets, and lifetime. |
| Codec/playback/debug CVars from day one | Creates unsupported combinations and test burden without an accepted content requirement. |

## Completion Definition

Geometry-cache animation is complete only when all of the following are true:

- Alembic is tools-only and produces a deterministic native cooked asset;
- unsupported topology or material data fails explicitly at cook time;
- GameFramework holds only playback state and immutable asset identity;
- sample bytes stream through one bounded residency/lifetime owner;
- raster, dynamic BLAS, and ray-hit attributes consume one current/previous deformed-geometry product;
- raster and ray-traced primary visibility, reflections, GI, and path/reference lighting show the same accepted deformation;
- D3D12 and Vulkan pass the recorded gates;
- Modern Sponza Knight is correctly scaled, placed, material-bound, animated, and measured without a scene-specific branch or static fallback;
- no USD runtime, Alembic runtime, pose bake, morph-frame expansion, duplicate cache, dormant mode, or permanent diagnostic clutter remains;
- documentation and workload status distinguish implemented evidence from future changing-topology and simulation work.
