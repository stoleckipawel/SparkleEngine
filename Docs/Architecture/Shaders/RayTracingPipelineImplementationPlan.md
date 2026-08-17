# Ray-Tracing Pipeline and Dual-Execution Delivery Plan

Status: implementation plan; not proof of implementation, runtime support, performance, or shipment
Date: 2026-08-17
Scope: complete ray-tracing shader-stage support, native D3D12/Vulkan ray-tracing pipelines, shader binding tables, frame-graph execution, effect-level inline/pipeline selection, lifetime, validation, and evidence

## Purpose And Authority Boundary

This plan turns SparkleEngine's existing inline ray-query renderer and compiler-only ray-tracing-library metadata into a complete, paired D3D12/Vulkan ray-tracing pipeline. It covers ray-generation, miss, closest-hit, any-hit, intersection, and callable shaders; native pipeline/state-object creation; shader identifiers/group handles; shader binding tables (SBTs); `TraceRays`; frame-graph integration; effect selection; hot reload; fallback; and evidence.

The plan does **not** claim that those systems exist today. Code, tests, executable build configuration, runtime captures, and measured evidence remain the authority for implemented behavior.

Authority is intentionally split:

- [Shader Authoring and Cooked Program Architecture](ShaderAuthoringAndCookedPrograms.md) owns shader identity, registration, compilation, cooking, publication, runtime program composition, and general pipeline-cache direction.
- [Renderer/RHI Boundary](../RendererRhiBoundary.md) owns the dependency boundary between renderer policy and backend mechanism.
- This document owns the delivery order, dual-execution contract, ray-tracing-specific ownership decisions, package gates, and completion evidence.
- [External Renderer Repository Comparison](../ExternalReferences/ExternalRendererComparison.md) owns the broad source-backed comparison. The references near the end of this plan are the narrower sources used for ray tracing.
- [Graphics Engineering Standard](../../Engineering/Standards/GraphicsEngineering.md), [Integration Style Guide](../../Engineering/Standards/IntegrationStyleGuide.md), [Change Process](../../Engineering/Standards/ChangeProcess.md), and [Validation, Performance, and Evidence](../../Engineering/Standards/ValidationPerformanceAndEvidence.md) govern implementation and review.

This plan advances `PGE-02`, `PGE-05`, `PGE-06`, and `PGE-09` from [Strategy Requirements](../../Strategy/Requirements.md). It does not replace their workload and evidence gates.

## Outcome

The completed system must let a selected ray-traced **effect** run through an inline ray-query implementation or a traditional ray-tracing pipeline implementation without changing its scene, material, output, or temporal contract.

That statement is deliberately narrower than "run any shader either way." Inline ray query and pipeline ray tracing have different invocation, control-flow, payload, attribute, recursion, and binding ABIs. A compute entry point cannot be turned into a ray-generation or hit entry point by a runtime switch. Portability belongs at the effect contract:

```text
                         Renderer effect request
                 scene + view + material + outputs + policy
                                      |
                         immutable execution plan
                         /                    \
              Inline frontend            Pipeline frontend
              compute dispatch            ray-generation dispatch
              RayQuery loop               TraceRay + RT stages
                    \                        /
                     shared semantic kernels
             ray setup, hit reconstruction, material lookup,
                 alpha decision, BSDF, output encoding
                                      |
                       identical effect output contract
```

An effect supports both modes only when it supplies and proves both frontends. Shared HLSL code contains pure data/semantic work; stage-only intrinsics remain in thin execution-specific entry files.

The product-level control is:

```cpp
enum class RayTracingExecutionMode : std::uint8_t
{
    Automatic,
    Inline,
    Pipeline,
};
```

Exact code names may change during implementation, but the closed three-state meaning must not. A frame resolves the requested mode once, before adding effect passes, into an immutable plan that records the active mode and reason for every selected effect.

## Non-Goals

- Do not emulate ray-generation, miss, hit, intersection, or callable stages inside one generic compute shader.
- Do not force every effect to use every legal ray-tracing stage.
- Do not create an Unreal-scale material/vertex-factory/plugin framework.
- Do not copy Unreal, NVRHI, Cauldron, or native API class names into Sparkle.
- Do not expose D3D12 state-object pointers, Vulkan pipeline handles, or raw shader identifiers to the Renderer.
- Do not build two acceleration-structure or material systems for the two execution modes.
- Do not advertise runtime pipeline support because a compiler can produce a library.
- Do not silently fall back from an explicitly requested mode.
- Do not require an SBT rebuild every frame when its logical records and exact pipeline generation are unchanged.
- Do not put material structures, descriptor tables, or large per-instance payloads into local SBT records.
- Do not add recursive ray depth, callable shaders, procedural geometry, pipeline libraries, or GPU-generated SBTs to product effects without a workload that benefits from them.

## Terms And Invariants

| Term | Meaning in this plan |
| --- | --- |
| Inline | A graphics or compute shader explicitly traverses an acceleration structure through `RayQuery`, `TraceRayInline`, and `Proceed`. |
| Pipeline | A ray-generation shader invokes `TraceRay`; the native ray-tracing pipeline schedules miss, hit-group, and optional callable stages. |
| Effect | A renderer operation with one scene/input/output/quality contract, for example ray-traced G-buffer or shadow visibility. |
| Frontend | The thin inline or pipeline-specific shader and pass code that invokes shared effect semantics. |
| Program | A typed composition of exports, hit groups, binding layout, payload/attribute contract, recursion policy, and permutations. |
| SBT plan | Renderer-owned logical selection and indexing of ray-generation, miss, hit-group, and callable records. |
| Shader table | RHI-owned, backend-materialized GPU records containing identifiers/group handles from one exact native pipeline generation plus small local data. |
| Pipeline generation | An immutable native RT pipeline version. Every materialized shader table identifies the generation that produced its identifiers. |

The invariants are:

1. One effect owns one public input/output contract, regardless of execution mode.
2. Inline and pipeline frontends share semantic kernels and data schemas, not stage intrinsics.
3. One render scene and one TLAS generation serve both modes.
4. Renderer policy selects the mode; RHI reports capability and performs native mechanism.
5. Explicit `Inline` and `Pipeline` requests are strict. An unsupported request produces a precise readiness error before graph execution.
6. `Automatic` may select per effect, but the resulting mixed plan is visible in diagnostics and captures.
7. A pipeline is usable only when compiler target, cooked artifact, runtime API feature, stage set, binding ABI, and effect implementation are all ready.
8. An SBT record can execute only with the exact live pipeline generation from which its identifier/group handle came.
9. TLAS instance contribution, geometry index, ray type, SBT record order, and shader-side trace parameters follow one documented formula.
10. Resources and pipeline/table generations retire by GPU submission token, never by CPU frame age.

## Current-Code Reconciliation

The 2026-08-17 audit found a useful partial vertical slice, not an empty system.

| Area | Current source-backed state | Disposition |
| --- | --- | --- |
| Inline traversal | `Engine/Assets/Shaders/RayTracing/RayTracingTraceQuery.hlsli` uses `RayQuery<RAY_FLAG_NONE>`, `TraceRayInline`, and `Proceed`; current G-buffer, direct-shadow, path-traced, and ReSTIR work executes as compute. | Preserve as a first-class execution mode and parity oracle. |
| Effect selection | G-buffer selects `Rasterized` or `Raytraced`; `Raytraced` currently means inline compute. Lighting modes similarly select renderer algorithms rather than RT execution APIs. | Separate the effect/algorithm choice from `RayTracingExecutionMode`. Rename ambiguous user-facing/debug labels during migration. |
| RT shader types | `GlobalShader.h` defines ray-generation, miss, closest-hit, any-hit, intersection, callable, and hit-group types plus registration macros. | Complete and test the existing authoring path; do not introduce a second registration framework. |
| Cooked schema | RT libraries, six export kinds, triangle/procedural hit groups, local parameter records, payload size, attribute size, and recursion depth already exist. | Treat as compiler-only scaffolding until every runtime gate in this plan is met. Evolve the schema in place. |
| Runtime validation | Valid RT metadata is deliberately rejected with "runtime RT state object execution is not enabled yet." | Keep the rejection until `P4-GATE`; replace it with capability/ABI validation in the same change that enables execution. |
| Renderer registrations | No renderer/runtime use of `IMPLEMENT_RAY_TRACING_SHADER` or `IMPLEMENT_RAY_TRACING_HIT_GROUP` was found. | Add one conformance library first, then typed effect programs. |
| Compiler targets | DXC advertises DXIL RT libraries and both inline targets; SPIR-V RT library support is not advertised. RT libraries omit `-E`; current reflection skips RT libraries. | Close DXIL/SPIR-V compile, validation, inspection, and reflection/contract gaps before native execution. |
| Local parameters | Cooked local-parameter records exist, but the writer path does not yet establish a complete production path for them. | Either generate and validate bounded local records or explicitly require none in the first slice. Never emit plausible empty metadata accidentally. |
| RHI capability | `SupportsRayTracing` currently serves acceleration-structure meaning; `SupportsInlineRayQuery` is separate. D3D12 and Vulkan probe relevant features and expose alignment/limit data. | Replace ambiguous capability meaning with independent acceleration-structure, inline-query, and RT-pipeline readiness. |
| Native execution | No owned `ID3D12StateObject`, `SetPipelineState1`, `DispatchRays`, `vkCreateRayTracingPipelinesKHR`, `vkGetRayTracingShaderGroupHandlesKHR`, or `vkCmdTraceRaysKHR` path was found. | Add behind backend-neutral RHI contracts, paired across D3D12 and Vulkan. |
| Pipeline service | Public RHI creates graphics and compute pipelines only. | Extend the existing service with RT pipeline creation; do not create an unrelated service locator. |
| Commands | `RenderCommandList` binds graphics/compute state and records draw/dispatch plus AS work; it has no RT pipeline/SBT binding or trace command. | Add typed RT binding and `TraceRays`; validate queue and resource state. |
| Frame graph | Pass kinds and typed runtimes cover raster, compute, transfer, and external-provider work. | Add a typed ray-tracing pass and graph resource declarations before direct use in renderer effects. |
| TLAS indexing | Public classic/partitioned descriptors and both native builders preserve `InstanceContributionToHitGroupIndex`; renderer TLAS builders currently write zero. | Extend the current renderer scene owner to compute the accepted SBT contribution. Do not add a second instance table. |
| Lifetime/cache | Existing render-pass runtime/cache and submission-token retirement patterns can own new objects, but do not understand pipeline/SBT generations. | Extend those owners and prove stale-generation rejection and GPU-safe retirement. |

### Current path

```text
typed compute registration -> DXIL/SPIR-V cook -> compute pipeline
                                                |
renderer effect -> frame-graph compute pass -> Dispatch
                                                |
                                       RayQuery + shared TLAS

typed RT metadata -> RT-library cook/inspection -> deliberate runtime rejection
```

### Target path

```text
typed effect program
  |-- inline CS ----------> graphics/compute pipeline ----> Dispatch
  `-- RT exports/groups --> native RT pipeline generation -> SBT generation -> TraceRays
                  \____________ shared layout/ABI _______________/

renderer effect request -> resolve mode -> add exactly one frontend pass
                                      -> shared TLAS/material/geometry/output
```

## External Reference Conclusions

| Source | Adopt | Do not copy |
| --- | --- | --- |
| Unreal Engine RHI | Separate compile support from runtime enabling; represent raygen/miss/hit/callable tables explicitly; use one higher-level scene/SBT model that can support inline and RTPSO consumers; resolve feature policy before dispatch. | Unreal's macro volume, renderer scale, material system, or naming. Unreal documentation supports shared infrastructure and policy selection, not arbitrary shader interchangeability. |
| NVIDIA NVRHI | Keep AS usable by both paths; construct an explicit RT pipeline and shader table; set state then dispatch; distinguish logical export names from native group handles; version mutable tables. | A second abstraction layer parallel to Sparkle RHI or NVRHI ownership conventions that conflict with local boundaries. |
| NVIDIA RTXPT | Use a compact RT pass as evidence that full path-tracing work can be organized around an explicit pipeline on D3D12/Vulkan. | RTXPT's application architecture or an assumption that every Sparkle effect should become one monolithic RT pass. |
| AMD Cauldron/FidelityFX | Gate acceleration structures and ray-query use explicitly; keep inline tracing in small common helpers with effect-provided hit/miss semantics; keep application integration owned by the renderer. | Treating AMD's inline denoiser sample as proof of RT pipeline support or copying a framework module wholesale. |
| Microsoft DXR | Model state objects, exports, hit groups, identifiers, table regions, `DispatchRays`, and the hit-group index formula exactly; reject out-of-bounds construction before GPU execution. | Leaking native root signatures/state subobjects into Renderer types. |
| Khronos Vulkan RT | Build shader stages/groups and query group handles from the exact bound pipeline; enforce base/stride/handle alignment and device-address requirements exactly. | Assuming D3D12 table layout values are portable to Vulkan. |

## Target Ownership

| Owner | Owns | Must not own |
| --- | --- | --- |
| Shader Tools | Compile target capability, library export discovery/validation, hit-group legality, layout/ABI metadata, deterministic artifacts, inspection, source diagnostics. | Native GPU pipelines, scene policy, SBT lifetime. |
| RHI public contract | Independent capabilities; immutable RT pipeline descriptor; opaque pipeline/table handles; logical table materialization request; trace descriptor; resource states; validation-facing errors. | Effect names, material policy, frame-graph scheduling, native handles. |
| D3D12/Vulkan RHI | Native pipeline/state object, layout/root association, identifier/group-handle retrieval, record packing/alignment, GPU table storage, command encoding, backend validation, generation retirement mechanics. | Which material/ray type an SBT slot means. |
| Renderer shader/program layer | Typed export/hit-group composition, effect ABI, global binding layout, payload/attribute schemas, recursion policy, permutation selection. | Raw native identifier bytes. |
| Renderer scene owner | One AS generation, instance/geometry/material identity, logical SBT contribution plan, dirty generation, scene readiness. | Backend table strides or native object construction. |
| Renderer frame/effect owner | Requested/active mode, readiness reason, exactly one frontend pass, shared inputs/outputs, fallback and telemetry. | Backend feature queries scattered through effects. |
| Frame graph/runtime cache | RT pass resource declarations, queue compatibility, pre-execution materialization, pipeline/table generation reference, submission-token lifetime. | Shader authoring or scene semantic identity. |

## Effect Dual-Execution Contract

Each migrated effect supplies an implementation record equivalent to:

```text
Effect identity
Shared input/output parameter schema
Shared semantic include set and ABI version
Inline implementation: typed CS or graphics shader, dispatch shape, readiness
Pipeline implementation: typed raygen + miss[] + hit groups[] + callable[], trace shape, readiness
Required scene features: triangles, alpha test, procedural, ray types
Quality/permutation constraints
Validation oracle and tolerance policy
```

The record is generated or declared next to the typed shader program. It does not become a general plugin registry.

### Selection semantics

| Requested mode | Inline implementation ready | Pipeline implementation ready | Result |
| --- | ---: | ---: | --- |
| `Inline` | yes | any | Inline. |
| `Inline` | no | any | Readiness error naming effect and missing inline requirement. No silent switch. |
| `Pipeline` | any | yes | Pipeline. |
| `Pipeline` | any | no | Readiness error naming effect and missing export/backend/table requirement. No silent switch. |
| `Automatic` | yes | yes | Select through one renderer policy using measured device/effect evidence; record reason. |
| `Automatic` | yes | no | Inline, with pipeline-unavailable reason. |
| `Automatic` | no | yes | Pipeline, with inline-unavailable reason. |
| `Automatic` | no | no | Use the effect's already accepted non-RT fallback, or reject the effect if it has none. |

A global strict `Inline` or `Pipeline` request must report all selected effects that cannot honor it before graph execution. `Automatic` may produce a mixed plan, but capture labels and diagnostics must display each effect's active mode. This provides the practical whole-pipeline switch without pretending that every effect has identical shader entry points.

### Shared HLSL boundary

Share:

- ray construction inputs and normalized ray description;
- instance, geometry, primitive, vertex, material, and transform lookup;
- triangle barycentric interpolation and hit reconstruction;
- alpha-mask decision and material evaluation;
- BSDF/light-transport kernels that do not call traversal intrinsics;
- surface/output structures and encoding;
- constants, stable resource indices, and numerical conventions.

Keep frontend-specific:

- thread/dispatch indexing versus `DispatchRaysIndex`/`DispatchRaysDimensions`;
- `RayQuery`, candidate loops, candidate commit, and inline miss selection;
- `TraceRay`, payload mutation, built-in attributes, `IgnoreHit`, `AcceptHitAndEndSearch`, and `CallShader`;
- ray flags where API semantics differ;
- recursion and stack behavior;
- local SBT record access.

The first effect should use a source layout equivalent to:

```text
Passes/RayTracing/RaytracedGBufferShared.hlsli
Passes/RayTracing/RaytracedGBufferInline.hlsl
Passes/RayTracing/RaytracedGBufferPipeline.hlsl
```

The exact split is decided by code review. It must leave one owner for hit reconstruction and output encoding.

## Pipeline, ABI, And Shader-Table Contract

### Stage support

Full support means every legal stage can be compiled, cooked, inspected, validated, composed, materialized, dispatched, captured, and tested:

| Stage | Required product use | Required proof |
| --- | --- | --- |
| Ray generation | Every pipeline program. | Dispatch dimensions, output bounds, global binding, payload initialization. |
| Miss | At least one per program/ray type as declared. | Miss-index selection, output/payload behavior, table bounds. |
| Closest hit | Triangle and procedural hit groups when selected. | Attribute/payload ABI, instance/geometry/material identity. |
| Any hit | Only for effects needing alpha/visibility semantics. | Ignore/accept behavior and parity with inline candidate filtering. |
| Intersection | Only for procedural AABB geometry. | Attribute production, bounds, hit ordering, interaction with any/closest hit. |
| Callable | Optional product use; complete runtime support still required. | Callable-table index, parameter ABI, bounds, and a focused conformance workload. |

An opaque triangle effect does not add empty any-hit, intersection, or callable stages merely to tick a box. Those stages are proven with later focused workloads.

### Binding and local data

- One typed global binding layout remains authoritative across all exports in a program unless a measured case justifies local association.
- The first vertical slice uses global/bindless resources and zero local parameters.
- Later local records contain only bounded POD such as stable material/geometry indices or small constants.
- Descriptors, owning pointers, variable-sized objects, or duplicated material data do not live in an SBT record.
- A local binding layout has an explicit hash and owning export/hit group. The cooker and runtime validate both.
- Payload and attribute layouts have named C++/HLSL contracts, byte size, alignment assumptions, hash/version, and stage visibility.
- Pipeline recursion depth is the minimum required by the effect and is validated against device capability. The first G-buffer slice uses depth one.

### SBT organization and index formula

The logical layout is:

```text
ray-generation region: exactly one selected record for one dispatch
miss region:           miss[rayType]
hit region:            hit[instance/geometry/rayType mapping]
callable region:       callable[logical callable index]
```

For a trace call, the hit-group record follows the DXR-equivalent formula:

```text
recordIndex =
    rayContributionToHitGroupIndex
  + multiplierForGeometryContributionToHitGroupIndex * geometryIndex
  + instanceContributionToHitGroupIndex
```

The Vulkan frontend maps the same logical result to `sbtRecordOffset`, `sbtRecordStride`, and the TLAS instance `instanceShaderBindingTableRecordOffset`. The Renderer owns the logical terms and proves bounds. The backend owns byte offsets, addresses, handle size, stride, and alignment.

The initial one-ray-type/one-opaque-hit-group slice uses all-zero contributions intentionally. The next slice introduces material/visibility ray types and makes the formula nontrivial before general migration.

### Generation and lifetime

```text
cooked program generation
          |
native RT pipeline generation
          |
queried identifier/group handles
          |
materialized SBT generation
          |
frame-graph pass reference -> submission token -> retirement
```

- Pipeline creation produces a monotonically distinguishable generation identity.
- Shader-table materialization stores the pipeline generation and refuses records from another generation.
- Reload publishes a new pipeline/table pair atomically for future frames.
- In-flight frames keep old pairs alive until their last submission token completes.
- Scene/SBT logical changes create a new immutable table generation or bounded dirty update; readers never observe partial writes.
- An SBT cache key includes program/permutation identity, native pipeline generation, logical record content, backend, and every layout-affecting capability.
- Pipeline readiness is materialized/prewarmed before frame-graph execution. No first-use compilation is hidden inside `Execute`.

## Delivery Rules

1. Phases are ordered. Packages inside a phase can be split into reviewable changes only when their gate remains false until the complete phase lands.
2. A backend-only intermediate change may exist behind compiler/test-only status, but user-visible runtime support remains disabled until paired D3D12/Vulkan execution passes.
3. Tests and failure injection are written before the production path they protect.
4. Every phase extends an existing owner or explicitly replaces and deletes the old authority.
5. Every change records producer, consumer, lifetime, build membership, and failure behavior.
6. Every runtime-enabling change includes negative tests. A successful screenshot alone is not acceptance.
7. No phase weakens the current inline path or the deliberate RT-library runtime rejection without satisfying its replacement gate.
8. Each phase ends with `git diff --check`, the smallest selected builds/tests, and any architecture check required by touched Renderer/RHI boundaries.

Package states are `Unselected`, `Selected`, `In progress`, `Evidence ready`, and `Accepted`. A package is `Accepted` only after its exit evidence has been reviewed.

## Phase 0 — Freeze The Contract And Baseline

### Test it first

- Capture the current shader compiler/backend capability listing for DXIL and SPIR-V.
- Prove current inline ray-traced G-buffer execution on D3D12 and Vulkan with the same map, camera, settings, and outputs.
- Record the current intentional runtime rejection for a valid RT-library package.
- Add source-level absence checks for renderer RT registrations, native RT pipeline creation, identifier retrieval, and trace commands.
- Inventory every current ray-query effect, its outputs, ray flags, alpha behavior, recursion needs, and accepted non-RT fallback.

### Outcome

Reviewers have a frozen current-state ledger, selected first workload, exact dual-execution contract, and no ambiguous claim that compiler metadata is runtime support.

### `RT0-GATE` required work

- Select `RaytracedGBuffer` as the first parity effect and one deterministic opaque-triangle map/camera as its minimal route.
- Define requested versus active mode and readiness-reason diagnostics.
- Define payload, triangle attributes, output formats, miss values, ray flags, instance mask, and numerical comparison policy.
- Record how existing `GBufferMode::Raytraced` is renamed or decomposed so "raytraced" no longer implies one execution API.
- Record the one owner and removal plan for every touched capability, program, pass, scene mapping, cache, and setting.

### Positive guardrails

- Keep the existing inline path runnable as the comparison oracle.
- Use the same scene/TLAS/material buffers for both paths.
- Make unsupported combinations observable before frame-graph execution.

### Negative guardrails

- Do not create native RT objects.
- Do not change default rendering behavior.
- Do not add a permanent compatibility alias for an ambiguous mode name.

### Exit gate

`RT0-GATE` passes when the ledger and baseline artifacts can be reproduced, the first workload is frozen, and reviewers agree on the effect-level—not arbitrary-entry-point—portability boundary.

### Ready-to-use implementation prompt

> Implement Phase 0 of `RayTracingPipelineImplementationPlan.md`. Add tests and evidence descriptors first. Reconcile current inline effects, shader metadata, capability meanings, owners, and deletion obligations. Freeze the Raytraced G-buffer parity route and mode/readiness contract. Do not enable native ray-tracing pipeline runtime behavior.

## Phase 1 — Prove The Complete Shader-Library Toolchain

### Test it first

- Add a tiny compiler conformance library containing all six export kinds, one triangle hit group, and one procedural hit group.
- Add rejection cases for duplicate export, missing export, wrong stage kind, illegal triangle intersection shader, missing procedural intersection shader, dangling hit-group index, incompatible layout, oversized payload/attribute, invalid recursion depth, and malformed local record.
- Require artifact inspection to list export kind/name/entry, groups, layout hashes, payload/attribute sizes, recursion, and binary target.
- Require DXIL validation and SPIR-V validation/inspection appropriate to the produced artifacts.

### Outcome

DXC can compile, validate, cook, inspect, and deterministically reproduce complete RT libraries for both runtime targets. Metadata is trustworthy but still runtime-disabled.

### `RTC-01` required work

- Advertise SPIR-V RT-library capability only after the actual DXC path succeeds.
- Preserve no-entry-point library compilation while discovering/validating every declared export.
- Make library reflection or an equivalent compiler-backed contract pass cover resources and exports instead of skipping the package.
- Produce binding, export, hit-group, local-parameter, payload, attribute, recursion, target, backend, and source-identity records from one cook plan.
- Resolve whether local parameter records are generated; reject unsupported declarations rather than emit misleading empty data.
- Make codegen target and compiler capability failure precise at plan time.
- Keep Slang capability false or explicitly scoped until its own conformance suite passes; no inferred support.

### Positive guardrails

- Extend `ShaderContractCatalog`, cook planning, package writing, inspection, and existing metadata validation.
- Keep export identity separate from HLSL source filename and native group handle.
- Make all order-dependent output deterministic.

### Negative guardrails

- Do not remove the runtime rejection.
- Do not accept a library because DXC returned success while declared exports or ABI records are missing.
- Do not equate one DXIL success with SPIR-V support.

### `RT1-GATE` exit

Both targets pass the positive and adversarial conformance matrix; byte-identical inputs produce stable package identity; inspection exposes the complete contract; runtime still rejects execution.

### Ready-to-use implementation prompt

> Implement Phase 1. Begin with the six-stage RT-library conformance source and invalid-contract tests. Close DXC DXIL/SPIR-V capability, export, reflection/contract, cook, local-record, validation, and inspection gaps through existing Shader Tools owners. Preserve the deliberate runtime rejection and report exact commands and artifacts.

## Phase 2 — Add The Backend-Neutral RHI Contract

### Test it first

- Add pure contract tests for independent AS/inline/pipeline capabilities and invalid combinations.
- Add descriptor tests for missing raygen, duplicate exports, unknown group references, illegal group composition, layout mismatch, capability limit overflow, invalid dimensions, zero/overflowing record counts, and unsupported queue use.
- Add SBT layout tests parameterized by handle size, base alignment, record alignment, maximum stride, and region count.
- Add generation mismatch and use-after-retirement simulations with fake opaque handles.

### Outcome

Public RHI can describe and validate RT programs, tables, and trace work without exposing backend types. No native execution is advertised yet.

### `RTRHI-01` required work

- Split the ambiguous `SupportsRayTracing` meaning into explicit acceleration-structure, inline-ray-query, and ray-tracing-pipeline capability. Migrate all producers/consumers and remove the old field in the accepted change.
- Add immutable backend-neutral RT pipeline descriptors containing program bytecode records, export/group composition, global/local layout references, payload/attribute limits, recursion policy, and debug identity.
- Add opaque `RayTracingPipeline` and `RayTracingShaderTable` runtime products with generation identity.
- Add a logical shader-table materialization descriptor containing named program records and bounded local POD, never raw native identifiers.
- Add a `TraceRaysDesc` with raygen, miss, hit, callable regions and dispatch dimensions.
- Extend the existing pipeline/service and command-list owners instead of adding a parallel RHI.
- Define binding semantics, resource tracking, resource states, allowed queue types, and error reporting.

### Positive guardrails

- Keep Renderer vocabulary semantic and RHI vocabulary mechanical.
- Make all size/index arithmetic checked and 64-bit where byte-address overflow is possible.
- Make invalid descriptors fail before backend calls.

### Negative guardrails

- Do not expose `D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES`, `VkStridedDeviceAddressRegionKHR`, native handles, or raw handle bytes publicly.
- Do not overload graphics/compute `RenderPipeline` behavior if that obscures RT lifetime or binding requirements.
- Do not claim support from extension presence alone.

### `RT2-GATE` exit

Contract tests prove capability truth, descriptor legality, layout arithmetic, generation ownership, and command preconditions. Both backends compile against the neutral contract while reporting pipeline execution unavailable.

### Ready-to-use implementation prompt

> Implement Phase 2. Write capability, descriptor, SBT arithmetic, generation, and command-precondition tests first. Replace ambiguous ray-tracing capability meaning across existing owners. Add neutral immutable RT pipeline/table/trace contracts to the existing RHI services without native-handle leakage. Keep execution disabled.

## Phase 3 — Materialize Native D3D12 And Vulkan Pipelines

### Test it first

- Add backend construction tests using the conformance library and deliberately corrupt export/group/layout inputs.
- Add SBT packing tests that compare native requirements with the neutral arithmetic oracle.
- Add a minimal headless or offscreen GPU test that writes distinct raygen, miss, triangle-hit, procedural-hit, and callable sentinels.
- Run D3D12 debug/GPU validation and Vulkan validation with synchronization validation.
- Add a forced pipeline-generation reload between table build and dispatch; execution must reject the stale pair.

### Outcome

Both backends can create native RT pipelines, retrieve identifiers/group handles, materialize aligned shader tables, bind global state, and record a trace command through RHI. Renderer product effects remain disabled until frame-graph integration.

### `RTD3D-01` D3D12 work

- Build one state object from validated DXIL libraries, exports, hit groups, shader/pipeline config, global root signature, and bounded local-root associations.
- Query `ID3D12StateObjectProperties` identifiers and keep them private to the pipeline generation.
- Pack raygen/miss/hit/callable records with D3D12 identifier size, record alignment, and table alignment.
- Bind the state object and global layout, then encode `DispatchRays` through `RenderCommandList`.
- Name native objects and surface HRESULT/subobject context precisely.

### `RTVK-01` Vulkan work

- Load and validate all required KHR function pointers only when extensions, features, properties, and dependent device-address/AS features are ready.
- Build shader stages and general/triangle/procedural groups from validated SPIR-V exports.
- Create the RT pipeline with validated recursion depth and layout.
- Query shader-group handles from the exact pipeline generation.
- Pack device-addressable SBT regions using handle size/alignment, base alignment, and maximum stride from device properties.
- Encode `vkCmdTraceRaysKHR` with live regions and global descriptors through `RenderCommandList`.
- Name native objects and surface `VkResult` plus group/export context precisely.

### Shared work

- Keep identifier/group-handle bytes backend-private.
- Materialize immutable table generations through the RHI owner and track their GPU resources.
- Validate every region address, size, stride, alignment, count, and bound before recording.
- Add backend parity diagnostics: API, pipeline generation, table generation, export/group counts, table bytes, and dispatch dimensions.

### Positive guardrails

- Use the same conformance program and sentinel oracle on both APIs.
- Keep global resource binding aligned with existing RHI descriptor conventions.
- Retire pipeline/table generations by submission token.

### Negative guardrails

- Do not copy identifiers between pipelines or backends.
- Do not accept a Vulkan extension without its feature/property/dependency chain.
- Do not leave one backend as a permanent compatibility path.

### `RT3-GATE` exit

The complete stage sentinel workload passes on D3D12 and Vulkan with clean native validation, stale-generation rejection, correct table alignment/index bounds, and captured object/dispatch markers. The product capability remains unavailable until the graph owns execution.

### Ready-to-use implementation prompt

> Implement Phase 3 as paired native backend packages behind non-product capability. Add construction, corruption, packing, validation, generation-reload, and GPU sentinel tests first. Build D3D12 state objects and Vulkan RT pipelines, retrieve private identifiers/group handles, materialize aligned immutable tables, and record trace commands through the neutral RHI. Do not bypass RHI from Renderer.

## Phase 4 — Integrate Frame Graph, Runtime Cache, And Lifetime

### Test it first

- Add graph tests for RT pass kind, legal queues, declared TLAS/buffer/texture accesses, transitions, dependencies, culling, and diagnostic labels.
- Add runtime-cache tests for ready, pending, failed, reloaded, stale-table, and retired pipeline/table generations.
- Add a test proving pipeline/table materialization happens before `FrameGraph::Execute`.
- Add resource-lifetime tests spanning multiple in-flight frames and both success/failure paths.

### Outcome

A typed ray-tracing pass participates in graph compilation and execution with the same resource/lifetime discipline as raster and compute passes. The deliberate RT-library runtime rejection can now be replaced with real readiness validation.

### `RTRDG-01` required work

- Add an explicit ray-tracing pass kind and typed `TraceRays<TPass>` builder path.
- Add a ray-tracing pass runtime holding the validated binding layout, exact pipeline generation, exact shader-table generation, and dimensions/policy needed at execution.
- Extend render-pass program definitions/runtime storage for RT composition instead of encoding RT as compute.
- Declare TLAS, global resources, outputs, and SBT resources through graph/resource owners; compile correct states and dependencies.
- Restrict initial trace passes to the graphics queue unless both RHI contract and backend validation explicitly support another queue.
- Materialize/prewarm pipeline and table through the runtime cache before execute; surface readiness failures before submission.
- Track all bound resources and retire runtime generations through existing submission tokens.
- Replace the unconditional valid-library rejection with validation of runtime format, capability, program/layout ABI, limits, generation, and selected effect readiness.

### Positive guardrails

- Reuse typed pass parameters and existing binding-layout validation.
- Give trace passes semantic labels visible in PIX, RenderDoc, and backend markers.
- Keep frame-graph execution free of compiler, disk I/O, or first-use pipeline creation.

### Negative guardrails

- Do not issue trace commands from an `ExternalProvider` or generic compute callback.
- Do not hide table uploads from graph/resource lifetime tracking.
- Do not allow a graph pass to outlive the exact runtime generations it captured.

### `RT4-GATE` exit

The conformance trace is scheduled as a typed graph pass on both backends, all dependencies and transitions validate, cold materialization occurs before execute, reload/retirement tests pass, and runtime support is reported only for the complete path.

### Ready-to-use implementation prompt

> Implement Phase 4. Add graph and runtime-lifetime tests first. Introduce a typed ray-tracing pass and `TraceRays` path, extend existing program/runtime-cache owners, declare every resource, pre-materialize exact pipeline/SBT generations, and retire by submission token. Replace the compiler-only runtime rejection only when the paired graph path passes.

## Phase 5 — Deliver Opaque Ray-Traced G-Buffer Parity

### Test it first

- Freeze exact expected miss values and field-specific comparisons for base color, normal, material, emissive, subsurface, device Z, and motion vector.
- Add same-frame inline-versus-pipeline output capture on the selected deterministic opaque scene.
- Add edge cases for miss, back-face culling, near-plane/TMin, nonuniform transforms, indexed geometry, skinned/morphed data if selected, and resolution bounds.
- Add selection tests for all requested/available combinations and exact readiness messages.

### Outcome

The existing ray-traced G-buffer effect can run as inline compute or a ray-generation/miss/closest-hit pipeline while using the same scene, resources, shared hit reconstruction, and output contract.

### `RTEFX-01` required work

- Refactor the current HLSL so hit reconstruction, material lookup, motion computation, miss encoding, and output stores have one semantic owner.
- Keep the current compute entry as the inline frontend.
- Add raygen, miss, and opaque triangle closest-hit exports as the pipeline frontend.
- Register one typed RT program and hit group through the existing shader-authoring path.
- Use one ray type, recursion depth one, global resources, and no local SBT data.
- Add one renderer effect builder that receives the resolved execution plan and schedules exactly one frontend pass.
- Keep target creation, scene/TLAS handles, external buffers, and downstream consumers unchanged.
- Add active-mode and readiness-reason markers/diagnostics.

### Positive guardrails

- Compare both modes from identical immutable frame inputs, not consecutive moving frames.
- Use exact comparisons for integer identity/sentinel fields and documented tolerances for floating-point fields.
- Keep rasterized G-buffer as the accepted non-RT fallback.

### Negative guardrails

- Do not branch on execution mode inside one low-level pass execution callback.
- Do not duplicate output encoding or hit reconstruction.
- Do not add any-hit, procedural, callable, multi-ray-type, or recursive complexity yet.

### `RT5-GATE` exit

Both modes pass the deterministic output oracle and clean captures on D3D12/Vulkan; strict mode selection and fallback behavior are correct; no scene/material/output authority is duplicated.

### Ready-to-use implementation prompt

> Implement Phase 5. Write field-specific G-buffer parity and selection tests first. Split the existing shader into shared semantic kernels plus inline and RGS/MISS/CHS frontends, register one typed RT program, and have one effect builder schedule exactly one frontend from the immutable execution plan. Preserve the raster fallback and existing consumers.

## Phase 6 — Add Production Hit Semantics And Multi-Ray-Type Indexing

### Test it first

- Add alpha-tested foliage/cutout cases with accepted/rejected candidates, front/back faces, UV edges, opaque overrides, and miss-after-ignore behavior.
- Add a second ray type, such as shadow visibility, with distinct miss/hit sentinels.
- Test the full record-index formula over several instances, geometry segments, materials, and ray types, including maximum valid and first invalid values.
- Corrupt renderer TLAS contribution, geometry multiplier, trace contribution, and table order independently; each test must fail or visibly select the wrong sentinel before acceptance.

### Outcome

Pipeline and inline execution agree on alpha/material visibility and share a nontrivial, bounded scene-to-SBT mapping suitable for real effects.

### `RTHIT-01` required work

- Add any-hit export(s) for alpha-tested triangle hit groups.
- Make the shared alpha decision the single semantic owner; inline candidate filtering and pipeline `IgnoreHit`/accept behavior become thin adapters.
- Extend existing renderer classic and partitioned TLAS builders to write the accepted `InstanceContributionToHitGroupIndex` instead of constant zero.
- Define geometry-segment order and ray-type slot order in the renderer scene generation.
- Validate that material/geometry updates invalidate the logical SBT generation without unnecessarily rebuilding BLAS/TLAS when AS content is unchanged.
- Migrate one direct-shadow visibility effect to both execution modes to prove a second ray type and different payload/output shape.

### Positive guardrails

- Keep material and geometry data out of the SBT; store stable indices only if local data is needed.
- Make classic TLAS and partitioned TLAS use the same logical contribution plan.
- Expose table bytes, record counts, rebuild reason, and update time.

### Negative guardrails

- Do not encode pointer identity or transient descriptor addresses in logical records.
- Do not rebuild every table or TLAS every frame by default.
- Do not allow inline and pipeline alpha thresholds or texture LOD policy to drift silently.

### `RT6-GATE` exit

Alpha-tested G-buffer and shadow visibility agree across modes and APIs; every index term and bound is tested; classic/partitioned scene paths share one mapping; measured rebuild behavior is documented.

### Ready-to-use implementation prompt

> Implement Phase 6. Add alpha and multi-ray-type/index corruption tests first. Introduce AHS as a thin adapter over shared alpha semantics, extend existing TLAS builders with one logical SBT contribution plan, and migrate direct-shadow visibility as the second dual-mode effect. Measure table rebuilds and keep large data outside records.

## Phase 7 — Prove Intersection And Callable Support

### Test it first

- Add a deterministic procedural AABB primitive whose intersection shader reports known attributes and multiple candidate distances.
- Test procedural miss, closest valid hit, any-hit rejection, transformed instances, attribute-size bounds, and triangle/procedural group misuse.
- Add a callable shader with a tiny typed parameter/payload contract and several valid indices.
- Test callable out-of-bounds, wrong ABI version/hash, missing callable region, and recursion/stack limit rejection.

### Outcome

Intersection and callable stages are real runtime features on both APIs, not unused enum values. Their optional product use remains workload-driven.

### `RTADV-01` required work

- Add a focused procedural-geometry conformance effect with intersection plus closest-hit and optional any-hit behavior.
- Add the minimum scene metadata needed to associate procedural BLAS geometry with its logical hit group without disturbing triangle ownership.
- Add a callable conformance effect or debug workload that executes through the normal typed program, SBT, graph, and lifetime path.
- Validate attributes, payload/callable data, indices, stack/recursion policy, and capture labels.
- Document whether any shipping effect benefits from callable shaders. If none does, keep support conformance-only and say so.

### Positive guardrails

- Use conformance workloads small enough for exact sentinel validation.
- Keep procedural intersection math separate from backend code.
- Treat callable support as optional composition, not a mandatory table in every pipeline.

### Negative guardrails

- Do not add fake empty stages to unrelated effects.
- Do not increase global recursion/stack policy to satisfy one test without an explicit program requirement.
- Do not claim product value from API coverage alone.

### `RT7-GATE` exit

Procedural intersection and callable conformance pass on D3D12/Vulkan with positive, bounds, ABI, reload, and native-validation evidence. All six stage kinds now traverse the complete source-to-runtime path.

### Ready-to-use implementation prompt

> Implement Phase 7. Build exact procedural-AABB and callable conformance tests first. Exercise intersection, any/closest hit, attributes, callable indices/ABI, SBT regions, typed graph dispatch, reload, and retirement on both APIs. Keep stage use optional in product effects and document any absent product consumer honestly.

## Phase 8 — Migrate Eligible Effects And Ship The Switch

### Test it first

- Inventory each ray-query effect and classify it `Dual`, `Inline only`, `Pipeline only`, or `Non-RT fallback`, with a reason.
- Add plan-resolution tests for the entire selected frame under strict `Inline`, strict `Pipeline`, and `Automatic`.
- Add a test proving strict modes list every incompatible selected effect and schedule no partial frame.
- Add temporal, accumulation, denoiser, and history reset tests for each migrated effect.
- Add hot-reload tests while several effects share a pipeline/library or table generation.

### Outcome

Users can switch all eligible ray-traced effects coherently, see the actual per-effect result, and understand any retained specialized path. No deep pass contains hidden API selection.

### `RTSEL-01` required work

- Add one global requested execution setting in the existing renderer settings/CVar path.
- Resolve one immutable per-frame execution plan before frame-graph construction.
- Migrate selected path-traced direct/indirect, ReSTIR visibility, reference lighting, or other ray-query effects only when each has an accepted parity/quality oracle and a useful pipeline design.
- Keep shared semantic/data code and thin frontends for each migrated effect.
- Make `Automatic` policy data-driven and inspectable; record device capability, effect availability, performance policy, selected mode, and reason.
- Make strict mode failure actionable rather than silently mixing.
- Rename diagnostics and UI so algorithm (`ReferencePathTraced`, `ReSTIR`, G-buffer method) and execution API (`Inline`, `Pipeline`) are independent axes.
- Preserve accepted raster/no-ray fallbacks and test them on devices lacking the required feature set.

### Positive guardrails

- Select before pass creation and schedule exactly one implementation per effect.
- Allow an effect to remain inline-only when pipeline staging has no demonstrated benefit; report it honestly.
- Keep the frame plan stable for the frame and include it in capture/evidence metadata.

### Negative guardrails

- Do not require a global mega-pipeline containing every effect.
- Do not make `Automatic` a hidden vendor-ID table without measured, versioned evidence.
- Do not duplicate temporal/history ownership between frontends.

### `RT8-GATE` exit

Strict mode behavior, automatic selection, effect classifications, fallbacks, UI/diagnostics, histories, and hot reload pass on both backends. Every selected effect either runs both modes or exposes its explicit, reviewed limitation.

### Ready-to-use implementation prompt

> Implement Phase 8 for the selected effect set. Add whole-frame plan, strict-mode, temporal, fallback, and reload tests first. Resolve one immutable execution plan before graph construction, migrate only effects with accepted dual-mode contracts, keep algorithm and execution settings separate, and expose every active mode and reason.

## Phase 9 — Performance, Failure, And Release Evidence

### Test it first

- Define fixed hardware, driver, API, build, scene, camera, warm-up, sample count, and percentile protocol before collecting results.
- Define correctness thresholds and known nondeterministic fields before looking at comparisons.
- Schedule forced failures: unsupported device, cook target absent, bad export, bad group, pipeline creation failure, SBT allocation failure, stale generation, table overflow, device loss/recreation, shader reload, and fallback selection.
- Record expected PIX, RenderDoc, and vendor-profiler marker/object names before captures.

### Outcome

The system is releasable, reproducible, measurable, and honest about the workloads where each execution mode wins or remains unavailable.

### `RTFIN-01` required work

- Run paired D3D12/Vulkan correctness routes on the deterministic conformance scene, opaque and alpha G-buffer routes, shadow ray type, procedural geometry, callable fixture, and selected flagship workload.
- Collect CPU frame cost, GPU effect time, p50/p95/p99 frame time, cold/warm pipeline time, SBT build/update time, SBT bytes, pipeline/table cache hits, TLAS/BLAS work, memory high-water, and fallback events.
- Capture both execution modes with identical inputs in PIX where applicable, RenderDoc where supported, and Nsight/other vendor tooling where it adds causal evidence.
- Compare shader instruction/register/stack characteristics where tools expose them; do not infer architecture causes from timing alone.
- Verify validation layers/debug layers are clean and markers make pipeline generation, table generation, effect, and mode obvious.
- Test device recreation and shader hot reload with multiple frames in flight.
- Run architecture-boundary, build-membership, formatting, unit/integration/GPU, documentation, and evidence checks required by touched targets.
- Remove temporary capability flags, compatibility fields, dead inline/pipeline prototypes, duplicated semantic functions, and stale docs in the same accepted delivery.

### Positive guardrails

- Report both wins and regressions per effect/device/API.
- Separate compile/cold-start cost from steady-state execution.
- Keep raw capture manifests and exact reproduction commands with the evidence.

### Negative guardrails

- Do not sum GPU queues or compare nonmatching frames/scenes.
- Do not call a mode faster from one mean timing or one vendor.
- Do not claim all-stage support without intersection and callable runtime evidence.
- Do not call the feature shipped while either backend or accepted fallback gate is incomplete.

### Completion gate

The plan is complete only when:

- all six RT shader stages pass source-to-runtime conformance on D3D12 and Vulkan;
- at least the ray-traced G-buffer and shadow visibility effects pass dual-mode parity;
- strict and automatic whole-frame selection are correct and visible;
- native pipeline/SBT construction, graph execution, generation reload, and GPU-safe retirement are proven;
- alpha, multi-ray-type indexing, procedural intersection, and callable bounds/ABI tests pass;
- accepted no-ray/raster fallbacks remain functional;
- validation and capture evidence are clean and reproducible;
- performance claims include the defined percentile, memory, cold/warm, and table-update data;
- the old ambiguous capability/mode authorities and runtime rejection have been removed only after their replacements are accepted;
- documentation states any effect that remains single-mode and why.

### Ready-to-use implementation prompt

> Implement Phase 9. Freeze the measurement and failure protocol first, then run paired D3D12/Vulkan correctness, validation, capture, reload, fallback, and performance evidence across the conformance and selected product workloads. Remove transitional authority and stale docs. Do not make shipment or performance claims beyond the captured matrix.

## Per-Package Change Template

Every implementation package copied from this plan must include:

```text
Package ID and phase:
Selected workload/effect:
Current owner to extend:
Authority replaced and deletion obligation:
Producer -> product -> consumer:
Requested/active mode behavior:
Compiler targets and shader stages:
Global/local ABI and hashes:
Pipeline/SBT generation and retirement:
Frame-graph resources and queue:
Fallback and failure behavior:

Tests written first:
Positive cases:
Negative/corruption cases:
Backend parity cases:
Reload/lifetime cases:

Positive guardrails:
Negative guardrails:

Build/test/validation commands:
Capture/evidence paths:
Measured overhead and limits:
Known limitations:
Exit reviewer and result:
```

## Verification Matrix

| Layer | Required verification |
| --- | --- |
| Authoring/contracts | Typed export and group composition; all stage kinds; duplicate/missing/illegal relationships; stable identity. |
| Compiler/cooker | DXIL and SPIR-V libraries; export discovery; binding/ABI metadata; deterministic cook; inspection; malformed packages. |
| RHI pure tests | Capability truth; descriptor legality; checked arithmetic; table alignment/stride/bounds; generation mismatch. |
| Backend GPU | Native construction; identifiers/handles; all regions; all stages; clean D3D12/Vulkan validation; sentinel outputs. |
| Frame graph | Pass kind, resource declarations, state transitions, queue legality, culling/dependencies, pre-execution readiness. |
| Renderer selection | Strict/automatic matrix, whole-frame preflight, reason reporting, exactly one frontend, accepted fallback. |
| Effect parity | Same immutable inputs; exact identity/sentinel fields; documented float tolerances; miss/hit/alpha/motion/history. |
| Scene/SBT | Classic/partitioned TLAS mapping; instance/geometry/ray-type formula; bounds; dirty generation; table bytes/update. |
| Lifetime | Cold/warm cache, reload, stale table rejection, multiple frames in flight, submission-token retirement, recreation. |
| Performance/evidence | Reproducible D3D12/Vulkan routes, p50/p95/p99, CPU/GPU, memory, SBT/pipeline metrics, paired captures. |

The exact executable target names must be discovered from current CMake membership when each phase is selected. A handoff must list exact commands and results. A typical final verification set includes the smallest selected Shader Tools/RHI/Renderer tests, paired backend builds/runs, native validation, `architecture_boundary_check` for Renderer/RHI boundary changes, and:

```powershell
git diff --check
```

## Review Checklist

- Does the change extend one existing owner and delete any replaced authority?
- Is effect/algorithm selection separate from inline/pipeline execution selection?
- Is the execution plan resolved before graph pass creation?
- Are shared semantics actually shared, with stage intrinsics confined to frontends?
- Are AS, inline-query, and pipeline capabilities independent and truthful?
- Are every export, group, layout, payload, attribute, recursion, and local record validated?
- Are native identifiers/group handles private and tied to one pipeline generation?
- Are SBT arithmetic, bounds, alignment, and the instance/geometry/ray-type formula tested?
- Does the graph own every resource, transition, queue rule, and lifetime?
- Does reload atomically publish a new pipeline/table pair and retire the old pair by submission token?
- Do strict mode failures name every incompatible effect without scheduling a partial frame?
- Are no-ray/raster fallbacks still tested?
- Are both APIs and every claimed shader stage represented in evidence?
- Are performance conclusions scoped to exact hardware, driver, build, scene, and sample policy?

## Reference Map

Local/current-state sources:

- [Shader Authoring and Cooked Program Architecture](ShaderAuthoringAndCookedPrograms.md)
- [Renderer/RHI Boundary](../RendererRhiBoundary.md)
- [External Renderer Repository Comparison](../ExternalReferences/ExternalRendererComparison.md)
- [Strategy Requirements](../../Strategy/Requirements.md)
- [Graphics Engineering Standard](../../Engineering/Standards/GraphicsEngineering.md)
- [Validation, Performance, and Evidence](../../Engineering/Standards/ValidationPerformanceAndEvidence.md)

Primary external references:

- [Microsoft DirectX Raytracing functional specification](https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html)
- [Khronos Vulkan ray-tracing chapter](https://docs.vulkan.org/spec/latest/chapters/raytracing.html)
- [NVIDIA NVRHI programming guide](https://github.com/NVIDIA-RTX/NVRHI/blob/main/doc/ProgrammingGuide.md)
- [NVIDIA NVRHI tutorial](https://github.com/NVIDIA-RTX/NVRHI/blob/main/doc/Tutorial.md)
- [NVIDIA RTX Path Tracing](https://github.com/NVIDIA-RTX/RTXPT)
- [AMD FidelityFX Denoiser inline ray-tracing helper](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/main/Samples/Denoisers/FidelityFX_Denoiser/dx12/shaders/raytracing_common.hlsl)
- [AMD Cauldron2 ray-tracing render module](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/main/Kits/Cauldron2/dx12/framework/render/rendermodules/raytracing/raytracingrendermodule.cpp)
- [AMD Cauldron Vulkan ray-tracing capability separation](https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/blob/master/src/VK/base/ExtRayTracing.cpp)
- [Unreal Engine hardware ray tracing](https://dev.epicgames.com/documentation/unreal-engine/hardware-ray-tracing-in-unreal-engine)
- [Unreal Engine `RHISupportsInlineRayTracing`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RHI/RHISupportsInlineRayTracing)
- [Unreal Engine `FRayTracingPipelineStateInitializer`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RHI/FRayTracingPipelineStateInitiali-)
- [Unreal Engine `FRayTracingShaderBindingTableInitializer`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RHI/FRayTracingShaderBindingTableIni-)
- [Unreal Engine `RayTraceDispatch`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RHI/FRHIComputeCommandList/RayTraceDispatch)

These references support responsibility boundaries and API facts. Sparkle's own code, tests, captures, and measurements decide the final implementation and every performance claim.
