# Deferred GBuffer Decals Delivery Plan

Status: implementation plan; not proof of implementation or acceptance

Scope: staged delivery of decal authoring, scene/GPU data, deferred composition, ray-hit reuse, and validation

Architecture authority: [Deferred GBuffer Decal Pipeline](../../Architecture/Modules/Engine/Renderer/DeferredGBufferDecals.md)

Cross-system shader sequence: [Shader System Delivery Plan](../CrossModule/ShaderSystem.md)

Acceptance authority: [Deferred GBuffer Decals Acceptance Contract](../../Acceptance/Renderer/DeferredGBufferDecals.md)

This plan owns feature-local delivery order, dependencies, and phase exit sequence. It does not redefine decal semantics, own the final acceptance criteria, or prove that any phase is complete.

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
- enable and accept the feature first with `GBufferAlgorithm::Rasterized` on D3D12 and Vulkan;
- use existing GBuffer visualization and a single pass marker for inspection.

Exit: raster image tests pass for every channel group, overlap, near-plane/camera-inside volumes, projection edges, jitter, receiver opt-out, and zero-decal identity. The chosen tile plan meets the recorded CPU/GPU/memory budget on the fixture and an overlap stress case.

### Phase 3 - Ray-traced primary GBuffer parity

- exercise the same post-producer pass with `GBufferAlgorithm::RayTracing` under both strict Inline and strict Pipeline execution;
- reconcile any depth reconstruction or receiver-bit production difference at the producer adapters, not in a ray-specific decal shader;
- compare the raster and ray-traced primary results from the same camera, scene, material table, ordering, and output formats on both backends;
- keep `RayTracingGBuffer` base-hit reconstruction decal-free so primary decals are not applied twice.

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

### Phase 5 - Acceptance preparation and cleanup

- run the full raster/ray-primary/ray-secondary matrix on D3D12 and Vulkan;
- capture the fixed Modern Sponza cameras and one adversarial overlap/near-plane scene;
- run existing architecture, shader ABI, formatting, build, and workload checks selected by the touched standards;
- remove temporary guards, duplicate structs, abandoned planners, unused flags, and stale documentation discovered by the migration;
- reconcile the capability snapshot and architecture with proven behavior, record the acceptance result in its owning contract/report, and retire this plan when no active delivery consumer remains.

Exit: the candidate is ready for judgment under the [Deferred GBuffer Decals Acceptance Contract](../../Acceptance/Renderer/DeferredGBufferDecals.md), with one authored contract, one runtime decal table, one primary resolve, one shared shader composition path, and one selected secondary lookup. Unavailable checks and unsupported surfaces remain stated rather than hidden by fallbacks.
