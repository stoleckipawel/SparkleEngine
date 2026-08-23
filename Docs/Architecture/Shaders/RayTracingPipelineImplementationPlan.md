# Ray-Tracing Pipeline and Dual-Execution Target Architecture

Status: target proposal and semantic contract; not implementation authority or proof of runtime support
Current-state audit provenance: 2026-08-23 source reconciliation is recorded in [Shader Authoring and Cooked Shader Architecture](ShaderAuthoringAndCookedPrograms.md#ray-tracing-phase-0-extension)
Scope: ray-query versus native ray-tracing execution semantics, effect portability, ownership, capability truth, typed stage composition, shader binding tables, scene indexing, lifetime, fallback, and target completion invariants

## Purpose and authority boundary

This document describes the intended ray-tracing system as one coherent target. It answers what inline ray query and native ray-tracing pipelines mean, what they share, what must remain distinct, who owns each decision, how shader tables map scene identity to native records, and what makes an effect genuinely dual-execution.

It intentionally owns no implementation phases, prompts, phase references, CL boundaries, test order, or delivery gates. [Shader Authoring and Cooked Shader Architecture](ShaderAuthoringAndCookedPrograms.md#implementation-contract) is the single implementation authority for the shader frontend, compiler, global shader map, code library, RHI/backend pipeline, shader table, frame graph, effects, tooling, and evidence. Its [unified implementation reference map](ShaderAuthoringAndCookedPrograms.md#unified-implementation-reference-map) owns the actionable external references.

Code, tests, executable build configuration, runtime captures, and measured evidence remain the authority for implemented behavior. Until the unified plan proves the complete path, the existing inline-query renderer is current and native pipeline execution is a target.

Related authority:

- [Renderer/RHI Boundary](../RendererRhiBoundary.md) owns the dependency boundary between Renderer policy and backend mechanism.
- [Graphics Engineering](../../Engineering/Standards/GraphicsEngineering.md), [Integration Style Guide](../../Engineering/Standards/IntegrationStyleGuide.md), [Change Process](../../Engineering/Standards/ChangeProcess.md), and [Validation, Performance, and Evidence](../../Engineering/Standards/ValidationPerformanceAndEvidence.md) govern implementation and review.
- [Strategy Requirements](../../Strategy/Requirements.md) owns `PGE-02`, `PGE-05`, `PGE-06`, and `PGE-09`; this target does not replace their workload or evidence gates.

## Target outcome

A selected ray-traced effect can run through an inline ray-query frontend or a native ray-tracing-pipeline frontend without changing its prepared scene, TLAS, material/geometry data, view, output, temporal, or fallback contract.

```text
                         Renderer effect request
                 scene + view + material + outputs + policy
                                      |
                         immutable execution plan
                         /                    \
              Inline frontend            Pipeline frontend
              compute dispatch            ray-generation trace
              RayQuery loop               TraceRay + RT stages
                    \                        /
                     shared semantic kernels
             ray setup, hit reconstruction, material lookup,
                 alpha decision, BSDF, output encoding
                                      |
                       identical effect output contract
```

Portability belongs to the effect contract, not to arbitrary shader entry points. A compute shader cannot be switched into ray generation or a hit stage at runtime: invocation, control flow, payload, attributes, recursion, and binding ABI differ. An effect is dual-execution only when it owns and proves both thin frontends over shared semantics.

## Terms and invariants

| Term | Target meaning |
| --- | --- |
| inline | a graphics/compute shader traverses an acceleration structure with `RayQuery`, `TraceRayInline`, and `Proceed` |
| pipeline | a ray-generation shader calls `TraceRay`; a native RT pipeline schedules miss, hit-group, and optional callable stages |
| effect | one renderer operation with one input/output/quality/history/fallback contract, such as GBuffer or shadow visibility |
| frontend | thin inline-query or RT-stage code that invokes shared effect semantics |
| pipeline composition | typed exports, hit groups, binding layout, payload/attribute contract, recursion, and optional bounded local data for one native RT pipeline |
| logical table plan | Renderer-owned selection and indexing of ray-generation, miss, hit-group, and callable records |
| shader table | RHI-owned backend-materialized GPU records containing identifiers/group handles from one exact native pipeline generation plus bounded local POD |
| pipeline generation | immutable native pipeline version; every compatible table identifies the generation that produced its native identifiers |

The enduring invariants are:

1. One effect owns one public input/output/temporal/fallback contract regardless of execution mode.
2. Inline and pipeline frontends share semantic kernels and data schemas, never stage intrinsics.
3. One `RenderScene`, prepared scene, TLAS generation, material/geometry identity, and view serve both modes.
4. Renderer policy selects the mode before graph construction; RHI reports capability and performs mechanism.
5. Explicit `Inline` and `Pipeline` requests are strict. Unsupported requests fail with one actionable readiness result before any partial graph is scheduled.
6. `Automatic` may select per effect, but every active mode and reason is stable for the frame and visible in diagnostics/captures.
7. A pipeline is usable only when compiler target, map/library records, runtime API feature chain, stage composition, binding ABI, native pipeline, table, graph path, and selected effect are all ready.
8. A shader-table record can execute only with the exact live pipeline generation from which its identifier/group handle came.
9. TLAS instance contribution, geometry index, ray type, table order, and shader trace parameters follow one documented checked formula.
10. Map/library/pipeline/table/resource generations publish atomically and retire by all-queue submission token, never CPU frame age or device-idle convenience.

## Non-goals

- Do not emulate ray-generation, miss, hit, intersection, or callable stages inside a generic compute shader.
- Do not force every effect to support both modes or every legal RT stage.
- Do not create an Unreal-scale material, vertex-factory, plugin, or shader-program framework.
- Do not expose D3D12 state objects, Vulkan pipelines, native identifiers/group handles, addresses, strides, or record bytes to Renderer.
- Do not create a second scene, acceleration-structure, material, history, map, runtime-generation, or effect-settings system.
- Do not advertise pipeline support because compilation or metadata alone succeeds.
- Do not silently fall back from an explicitly requested mode or partially schedule a strict frame.
- Do not rebuild every shader table every frame when its logical content and exact pipeline generation are unchanged.
- Do not put descriptors, owning pointers, transient addresses, variable-size objects, or duplicated material data in local records.
- Do not require any-hit, intersection, callable, recursion, collections, pipeline libraries, GPU-generated tables, permutations, or precaching where a real effect or permanent conformance invariant does not need them.

## Current-to-target reconciliation

The revision-pinned current inventory lives in the implementation document. Its architectural meaning is:

```text
CURRENT

typed compute shader -> global registration/package -> compute pipeline
                                                    |
effect -> frame-graph compute pass -> Dispatch -> RayQuery + shared TLAS

generic RT metadata -> package cook/inspection -> deliberate runtime rejection


TARGET

concrete shader classes -> compile jobs -> GlobalShaderMap + CookedShaderLibrary
        |                                      |
        |                                      +-> compute/graphics runtime
        |
        +-> focused RT composition -> native RT pipeline generation
                                      -> shader-table generation
                                      -> typed graph TraceRays

effect request -> immutable execution plan -> exactly one frontend
                                      -> shared scene/TLAS/material/output/history
```

The target replaces the compiler-only RT package scaffolding rather than adapting it. RT declarations enter the final map/library representation only with their complete native and typed-graph consumer.

## Target ownership

| Owner | Owns | Must not own |
| --- | --- | --- |
| ShaderCompiler | target capability, export discovery/validation, hit-group legality, parameter/layout/payload/attribute/recursion/local metadata, deterministic map/library records, inspection, source diagnostics | native GPU objects, scene policy, table lifetime |
| `GlobalShaderMap` / `CookedShaderLibrary` | typed target lookup and validated code/ABI records for raster, compute, and RT stages | effect selection, native identifiers, table record meaning |
| RHI public contract | independent AS/inline/pipeline capabilities; immutable RT pipeline descriptor; opaque pipeline/table products; logical table materialization request; trace descriptor; states and validation errors | effect/material names, frame scheduling, native handles/bytes |
| D3D12/Vulkan private RHI | native pipeline/state object, layout association, identifier/group-handle retrieval, record packing/alignment, GPU table resources, command encoding, native validation | which material, geometry, ray type, or effect a logical slot means |
| Renderer shader/effect owner | concrete RT shader classes, focused typed composition, shared effect ABI/semantics, requested/active mode, exactly one frontend, output/history/fallback | raw identifier bytes, backend table layout, compiler process policy |
| `RenderScene` and RT scene capabilities | one AS generation, instance/geometry/material identity, logical table contribution plan, dirty generation, classic/partitioned TLAS parity | backend strides/addresses, effect selection, graph handles |
| frame graph / `RenderPassRuntimeCache` | typed trace resource declarations, queue/state/dependency rules, pre-execute pipeline/table materialization, exact generation capture, atomic reload, submission-token retirement | shader authoring, mutable scene semantics, editor policy |
| Application/Editor | one `Apply Changed` intent, bounded immutable status/provenance presentation, requested renderer setting | compiler scheduling, map mutation, RHI objects, native construction, table bytes |

## Effect-level dual-execution contract

Each dual effect has one semantic record, not a plugin registry or second pass framework:

```text
Effect identity
Shared prepared scene/view/material/input/output/history contract
Shared semantic HLSL owners
Inline frontend: typed compute/graphics shader and dispatch shape
Pipeline frontend: typed raygen + miss[] + hit groups[] + callable[] and trace shape
Required scene features and ray types
Readiness and accepted non-RT fallback
Correctness/quality comparison policy
```

### Selection semantics

The closed request vocabulary is:

```cpp
enum class RayTracingExecutionMode : std::uint8_t
{
    Automatic,
    Inline,
    Pipeline,
};
```

Exact placement may follow current Renderer settings style, but the meaning is fixed:

| Requested mode | Inline ready | Pipeline ready | Result |
| --- | ---: | ---: | --- |
| `Inline` | yes | any | inline |
| `Inline` | no | any | readiness failure; no graph scheduled |
| `Pipeline` | any | yes | pipeline |
| `Pipeline` | any | no | readiness failure; no graph scheduled |
| `Automatic` | yes | yes | one inspectable Renderer policy chooses and records reason |
| `Automatic` | yes | no | inline with pipeline-unavailable reason |
| `Automatic` | no | yes | pipeline with inline-unavailable reason |
| `Automatic` | no | no | accepted non-RT fallback or explicit effect failure |

A strict whole-frame request preflights every selected effect, lists every incompatibility, and schedules nothing if any selected effect cannot honor the request. `Automatic` may mix modes, but never hides the result. Algorithm choices such as raster/ray GBuffer, Reference/ReSTIR lighting, or denoising remain independent axes from the execution API.

### Shared HLSL boundary

Share one semantic owner for:

- ray construction inputs and normalized ray description;
- instance, geometry, primitive, vertex, material, and transform lookup;
- triangle barycentric interpolation and hit reconstruction;
- alpha-mask/material visibility policy;
- BSDF/light transport that does not invoke traversal;
- surface/output structures, motion/depth conventions, and encoding;
- stable resource indices, constants, and numerical conventions.

Keep frontend-specific:

- thread/dispatch indexing versus `DispatchRaysIndex`/`DispatchRaysDimensions`;
- `RayQuery`, candidate loops/commit, and inline miss selection;
- `TraceRay`, payload mutation, built-in attributes, `IgnoreHit`, accept/end-search, and `CallShader`;
- stage-specific ray flags, recursion/stack behavior, and local-record access.

Thin frontends are siblings beside their semantic effect owner. They do not duplicate hit reconstruction, material evaluation, output stores, or temporal policy.

## Shader authoring and pipeline composition

Every RT stage is an ordinary concrete shader class in the same global-shader system:

- nested `Parameters` owns shader-visible global input/output fields;
- implementation registration owns virtual source, entry/export, and stage;
- the frozen catalog owns immutable type metadata;
- compile jobs and the final map/library own target code and ABI records;
- `ShaderRef<Shader>` owns typed runtime lookup.

`RayTracingPipelineComposition` is the only RT-specific authoring composition. It exists because a native RT pipeline must relate several exports and hit groups. It contains typed references and policy—not code bytes, package names, source strings, native identifiers, graph resources, or effect runtime state.

A composition specifies only what is structurally required:

- one or more ray-generation choices, selected one per trace;
- miss shaders ordered by logical ray type;
- triangle/procedural hit groups naming typed closest-hit and optional any-hit/intersection shaders;
- optional callable shaders;
- one authoritative global parameter layout and explicit bounded local layouts when present;
- payload and attribute contracts;
- minimum required recursion policy and debug identity.

Compute and ordinary graphics never use this type. Graphics continues to name concrete vertex/pixel shader refs with the actual draw pipeline description. There is no universal `ShaderProgram`, `TShaderProgram`, pass-registration macro, or string-only export registry.

## Pipeline ABI and shader-table contract

### Stage support

Full pipeline support means every legal stage can traverse source, compilation, map/library, typed lookup/composition, native materialization, table region, graph dispatch, capture, reload, and retirement:

| Stage | Target use and proof |
| --- | --- |
| ray generation | required by every composition; proves dimensions, bounds, global binding, payload initialization |
| miss | at least one per declared ray type; proves miss index, payload/output, table bounds |
| closest hit | triangle/procedural groups as selected; proves attributes, payload, instance/geometry/material identity |
| any hit | only effects/groups needing alpha/visibility; proves ignore/accept parity with inline candidate filtering |
| intersection | procedural AABB conformance and any product procedural geometry; proves attributes, distances, group legality |
| callable | optional composition; permanent focused conformance proves region/index/ABI/bounds without forcing product use |

No product effect adds an empty stage merely to claim coverage. Procedural and callable support may remain conformance-only when no product workload benefits; that limitation is explicit.

### Binding and local data

- One typed global parameter layout is authoritative across composition exports unless a measured case justifies a local association.
- The first product GBuffer pipeline uses global/bindless resources and zero local data.
- Later local records contain only bounded POD such as stable material/geometry indices or small constants.
- Descriptors, pointers, variable-sized objects, transient addresses, and duplicated material data never enter a logical or native record.
- Each local layout has one structural identity and owning export/group; compiler and runtime validate both.
- Payload and attribute contracts have named C++/HLSL schemas, exact byte limits/alignment assumptions, and stage visibility.
- Recursion depth is the minimum required by the composition and validated against device limits; initial product effects use depth one.

### SBT organization and index formula

The logical regions are:

```text
ray-generation: exactly one selected record per dispatch
miss:           miss[rayType]
hit:            hit[instance/geometry/rayType mapping]
callable:       callable[logicalCallableIndex]
```

The hit record follows one checked logical formula:

```text
recordIndex =
    rayContributionToHitGroupIndex
  + multiplierForGeometryContributionToHitGroupIndex * geometryIndex
  + instanceContributionToHitGroupIndex
```

Vulkan maps the same logical result to `sbtRecordOffset`, `sbtRecordStride`, and TLAS `instanceShaderBindingTableRecordOffset`. Renderer owns logical terms, ordering, and bounds. Backends own addresses, byte offsets, handle sizes, strides, alignment, and region packing.

The opaque one-ray-type first effect intentionally uses zero contributions. The production alpha/shadow slice introduces nontrivial instance/geometry/two-ray-type indexing through one `RenderScene` plan shared by classic and partitioned TLAS. Material/geometry data remains in shared buffers; the table stores at most stable small indices.

## Capability and readiness contract

Capability is not one `SupportsRayTracing` boolean. The target distinguishes:

- acceleration-structure construction and binding;
- inline ray-query compilation and runtime execution;
- native RT-pipeline compilation, feature/property/function readiness, materialization, shader-table support, command encoding, and typed graph execution.

Pipeline readiness is true only when the full compiler-to-graph chain is usable for the selected target/effect. Extension presence, successful library compilation, generic stage enums, valid metadata, or one backend alone cannot advertise product support.

Every unavailable result identifies the missing owner-level requirement: compiler target, export/group ABI, backend feature/function, layout/limit, native pipeline/table creation, graph queue/resource rule, generation mismatch, effect frontend, or fallback.

## Runtime, graph, and lifetime contract

```text
active map/library generation
          |
typed RT composition
          |
native RT pipeline generation
          |
queried identifiers/group handles
          |
immutable shader-table generation
          |
typed graph TraceRays pass
          |
submission token -> retirement
```

- `RenderPassRuntimeCache` remains the one map/layout/pipeline/table materialization and generation owner.
- Pipeline/table materialization and validation happen before graph execution. Execute binds already declared/resolved state and records `TraceRays`; it performs no file I/O, lookup, allocation policy, compilation, or hidden discovery.
- The graph declares TLAS, global resources, outputs, table buffers, states, transitions, dependencies, culling, queue legality, and diagnostics.
- A table stores the exact pipeline generation whose identifiers it contains; a mismatch is rejected before dispatch.
- Reload validates a complete new map/library/pipeline/table set before atomic publication. In-flight passes retain the old set until every relevant queue submission completes.
- Scene logical changes create a new immutable table plan/generation or bounded dirty update. Readers never observe partial record content.
- Table reuse identity includes typed composition, shader/map generation, exact native pipeline generation, logical records, backend, and every layout-affecting capability. This is correctness reuse, not a precache/prewarm subsystem.

## Product effect contract

The first product effect is the ray-traced GBuffer:

- shared prepared scene/view/TLAS/material/geometry and existing GBuffer outputs;
- inline compute frontend and raygen/miss/opaque-triangle-closest-hit frontend;
- one ray type, recursion depth one, global parameters, no local data, zero contribution mapping;
- exact identity/sentinel comparisons and field-specific floating-point tolerances in the same frame;
- raster GBuffer remains the accepted non-RT fallback.

The production hit slice adds:

- alpha-tested any-hit as a thin adapter over shared alpha/material policy;
- shadow visibility as a second dual-execution ray type;
- nontrivial instance/geometry/ray-type mapping shared across classic and partitioned TLAS;
- dirty table generation without unnecessary BLAS/TLAS rebuild.

Every remaining current ray-query effect is classified `Dual`, `InlineOnly`, `PipelineOnly`, or `NonRtFallback`. Migration requires a useful pipeline design and accepted correctness/quality/history oracle. A global mega-pipeline and forced migration for API coverage are rejected.

## Target completion invariants

The target is realized only when implementation evidence proves all of the following together:

- all six RT stages traverse the final shader/map/library/runtime/graph path on D3D12 and Vulkan;
- native identifiers/group handles remain private and table regions/indexing/alignment/bounds match the exact pipeline generation;
- ray-traced GBuffer and shadow visibility pass same-frame dual-mode parity including alpha and fallback behavior;
- classic/partitioned TLAS share one logical contribution plan and no scene/material/history authority is duplicated;
- one immutable whole-frame plan implements strict/automatic selection and schedules exactly one frontend per effect;
- map/library/pipeline/table reload and device recreation preserve the previous accepted generation on failure and retire old state by submission token;
- capture/provenance follows shader/effect identity through source, compile job, map/code, composition, native pipeline/table, graph event, and symbols;
- paired correctness, failure, capture, and performance evidence uses fixed hardware/driver/build/scene/camera/settings/sample provenance;
- accepted raster/no-ray fallbacks remain functional and every single-mode effect is documented honestly;
- no package compatibility, compiler-only RT replacement, ambiguous capability/mode, backend/graph bypass, universal shader-program layer, duplicate owner, permanent migration diagnostics, or unearned precache/permutation framework remains.

The exact implementation prompts, phase ordering, validation matrix, references, and final evidence contract are intentionally centralized in [Shader Authoring and Cooked Shader Architecture](ShaderAuthoringAndCookedPrograms.md#implementation-contract).
