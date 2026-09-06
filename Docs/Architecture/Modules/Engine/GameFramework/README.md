# GameFramework Capability Inventory

Status: capability snapshot; current, but not release approval or runtime evidence

Snapshot: 2026-09-06 at committed `master` revision `8414b5dc`; `Engine/GameFramework` public/private source, CMake membership, Application/Renderer boundaries, and cooked loaders inspected; evidence `S` only

Scope: level lifecycle, cooked asset loading, world/ECS storage, camera/light/material/mesh/animation behavior, editing, publication, and render extraction

Owner: `Engine/GameFramework` / `SparkleGameFramework`

Evidence and disposition: [Capability Evidence Plan](../../../../Plans/CapabilityEvidence.md) and [First Release Acceptance Contract](../../../../Acceptance/FirstRelease.md)

## Module Boundary

GameFramework depends on Core, Platform, and Tasks but not Renderer or RHI. It owns simulation/world truth and publishes Renderer-neutral `RenderFrameSubmission` data. This separation is a current implemented boundary, not merely an intended layering rule.

## Level And Cooked-Asset Capabilities

| ID | Capability | State | Exact current coverage and limit | Evidence |
| --- | --- | --- | --- | --- |
| `GF-001` | Level documents | Implemented path | Text `.level` parser/writer covers name, authored camera, optional sky, authored lights, and one or more scene-asset IDs. It is not a general scene graph serialization format. | `S` |
| `GF-002` | Level registry/session | Implemented path | Discovers registered levels, initializes a startup level, queues one requested change, reports Reading/Decoding/Validating/Ready/Failed/Cancelled progress, emits change events, exposes diagnostics, and saves the active level. | `S` |
| `GF-003` | Asynchronous scene load | Implemented path | A Tasks graph reads cooked files, decodes asset payloads, validates the manifest/package, and commits only the current request into the world. Cancellation and request IDs prevent obsolete publication. | `S` |
| `GF-004` | Cooked-only runtime | Implemented path | Runtime loaders consume scene manifests, meshes, materials, skeletons, animations, textures by reference, and the scene registry. Source import libraries are not linked to GameFramework. | `S` |
| `GF-005` | Cooked format identification | Partial | Mesh, material, scene, skeleton, and animation files have distinct 32-bit magic values and bounded readers validate layout. The shared header contains only magic—no explicit schema version, endian marker, payload size/hash, or producer identity. | `S` |
| `GF-006` | Scene registry | Implemented path | Maps stable scene IDs to relative cooked-manifest paths with load/save/upsert/release operations. It is a flat ordered map, not a dependency database. | `S` |
| `GF-007` | File lifetime during load | Implemented path | `CookedAssetFileSet` owns opened/mapped asset files while decoder spans are consumed; decoded package ownership moves into world stores on commit. Repeat-load residency remains to be measured. | `S` |
| `GF-008` | Manifest validation | Implemented path | Loader/validator checks file presence, indices, references, payload consistency, and reports explicit load failure rather than silently constructing partial scene data. Exact malformed corpus coverage is unproven. | `S` |

## World, ECS, And Simulation

| ID | Capability | State | Exact current coverage and limit | Evidence |
| --- | --- | --- | --- | --- |
| `GF-009` | Entity identity/lifetime | Implemented path | Generation-aware entity IDs, registry-backed component storage, alive check, destruction, and deferred structural command commit. No public arbitrary entity/component creation API is exposed. | `S` |
| `GF-010` | Fixed component schema | Implemented path | 13 stable hashed schemas: LocalTransform, WorldTransform, CameraDerivedState, MeshInstance, Visibility, Camera, Light, AnimationState, MorphState, SkinningState, Name, AuthoredIdentity, and EditorMetadata. | `S` |
| `GF-011` | Typed queries and frozen structure | Implemented path | Read/write query descriptors declare system access; world structure is frozen for a system epoch and structural changes commit outside it. This is a purpose-built ECS, not a generic public ECS framework. | `S` |
| `GF-012` | Compiled system graph | Implemented path | Eleven stages execute camera movement; playback; pose; morph; skinning; morph commit; system-output commit; transforms; camera derived state; mesh extraction; extraction commit. Dependencies and parallel-range policies are compiled. | `S` |
| `GF-013` | Parallel world evaluation | Implemented path | Camera, animation, pose, transform, and extraction systems use explicit grain/serial/partition limits through Tasks. Deterministic numerical output under different worker counts is not yet evidenced. | `S` |
| `GF-014` | Transform hierarchy evaluation | Implemented path | Dirty local/world transforms propagate through hierarchy; full reevaluation is available after scene commit; inverse-transpose data is produced for rendering. | `S` |
| `GF-015` | Camera simulation | Implemented path | Camera input intent drives navigation; derived world matrix, direction, aspect, visibility, and active camera are published. Perspective and orthographic vocabulary exists, but importer/runtime coverage differs. | `S` |
| `GF-016` | Lighting model | Implemented path | Directional, point, spot, and rectangular lights with physical/intensity fields plus optional sky environment flow from level/import through read view and render submission. Capacity/rendering limits belong to Renderer. | `S` |
| `GF-017` | Material model and variants | Implemented path | Material descriptions and handles, scene material tables, named imported variant sets, active variant selection, and per-instance material rebinding are represented and published. Blend rendering remains unsupported downstream. | `S` |
| `GF-018` | Static/instanced meshes | Implemented path | Immutable mesh resources, static mesh instances, authored/shared instance-group metadata, visibility, transforms, material bindings, and structural create/update/destroy extraction. | `S` |
| `GF-019` | Skeletal animation | Implemented path | Skeleton hierarchy, animation clips, linear/step/cubic sampling, playback advance, pose evaluation, up to eight imported influences, and joint-matrix ranges reach Renderer. | `S` |
| `GF-020` | Morph animation | Implemented path | Per-target position/normal/tangent deltas, default/per-instance weights, animation weight channels, evaluated morph outputs, and render weight ranges reach Renderer. | `S` |
| `GF-021` | World edits | Implemented path | Generation-checked commands can set active camera, local transform, camera description, visibility, light description, sky, and active material variant; result is Accepted/Stale/Rejected with message. | `S` |
| `GF-022` | Read snapshots/change journal | Implemented path | Immutable shared `WorldReadView` snapshots expose cameras/lights/meshes/sky; sequence cursors read and acknowledge change batches. Retention/bounded-history behavior needs long-run evidence. | `S` |

## Render Publication Boundary

| ID | Capability | State | Exact current coverage and limit | Evidence |
| --- | --- | --- | --- | --- |
| `GF-023` | Structural publication | Implemented path | Scene generation/sequence, reset, object creates/updates/destroys, immutable material/texture tables, instance groups, and sky are published only when structurally required. | `S` |
| `GF-024` | Dynamic publication | Implemented path | Every frame can publish object transforms/visibility, all light descriptions, joint-matrix ranges/data, and morph-weight ranges/data. | `S` |
| `GF-025` | View publication | Implemented path | Camera data plus camera-cut/teleport flags are separate from scene data, allowing Renderer to invalidate view history without conflating scene ownership. | `S` |
| `GF-026` | Stable render identity | Implemented path | World entities map to generation-aware `RenderObjectId` values and immutable resource handles. Identity continuity across repeated load/reload remains an evidence item. | `S` |

## Vertical Frame Trace

Application publishes input -> `GameWorld::Update` freezes ECS structure and runs the 11-stage system graph -> system output commits changes -> `ExtractRenderFrameSubmission(frameId)` emits structural delta, dynamic arrays, and view input -> Application optionally replaces only view camera data for the editor -> Renderer consumes the moved submission and owns GPU publication.

## Explicit Non-Capabilities And Risks

- No physics, collision, audio, networking, scripting, gameplay class hierarchy, prefab system, generic public ECS authoring API, particle system, navigation mesh, or save-game system was found.
- Cooked assets have magic but no explicit version field; stale/incompatible rejection therefore depends on exact current layout validation and operational recooking.
- Runtime is cooked-only, but package independence, missing-registry behavior, repeated cancellation, and total decoded/GPU residency remain unproved.
- Editor-visible world edits cover a fixed property subset; entity creation, arbitrary component editing, material authoring, and mesh replacement are not current capabilities.
