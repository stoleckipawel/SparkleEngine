# Ray-Tracing Execution Architecture

Status: target architecture; semantic contract, not proof of runtime support
Current-state audit provenance: 2026-08-28 static source/build reconciliation at committed `master` revision `20814381`, whose source and executable build configuration are unchanged from implementation revision `99af6d5b`; migration history is recorded in the [Shader System Delivery Plan](../../../../../../Plans/CrossModule/ShaderSystem.md)
Scope: ray-query versus native ray-tracing execution semantics, effect portability, ownership, capability truth, typed stage composition, shader binding tables, scene indexing, lifetime, supported alternates, mandatory failure, and target completion invariants

## Purpose and authority boundary

This document describes the intended ray-tracing system as one coherent target. It answers what inline ray query and native ray-tracing pipelines mean, what they share, what must remain distinct, who owns each decision, how shader tables map scene identity to native records, and what makes an effect genuinely dual-execution.

It intentionally owns no implementation phases, prompts, CL boundaries, test order, or delivery gates. The [Shader System Delivery Plan](../../../../../../Plans/CrossModule/ShaderSystem.md#implementation-contract) is the single delivery authority for the shader frontend, compiler, global shader map, code library, RHI/backend pipeline, shader table, frame graph, effects, tooling, and validation sequence. Its [unified implementation reference map](../../../../../../Plans/CrossModule/ShaderSystem.md#unified-implementation-reference-map) owns the actionable external references; the [Shader System feature acceptance contract](../../../../../CrossModule/ShaderSystem/Acceptance.md) owns final proof.

Code, tests, executable build configuration, runtime captures, and measured evidence remain the authority for implemented behavior. Committed source contains dual-execution GBuffer and shadow routes plus the shared scene table plan, but native execution, parity, reload, and performance remain unproved under the feature acceptance contract.

Related authority:

- [Renderer/RHI Boundary](../../../../../Decisions/RendererRhiBoundary.md) owns the dependency boundary between Renderer policy and backend mechanism.
- [Renderer Engineering](../../../../../../Engineering/Modules/Renderer.md), [RHI Engineering](../../../../../../Engineering/Modules/RHI.md), [Change Integration](../../../../../../Engineering/Workflow/ChangeIntegration.md), [Change Lifecycle](../../../../../../Engineering/Workflow/ChangeLifecycle.md), and [Validation And Evidence](../../../../../../Engineering/Verification/ValidationAndEvidence.md) govern implementation and review.
- [Strategy Requirements](../../../../../../Strategy/Requirements.md) owns `PGE-02`, `PGE-05`, `PGE-06`, and `PGE-09`; this target does not replace their workload or evidence gates.

## Target outcome

A selected ray-traced effect can run through an inline ray-query frontend or a native ray-tracing-pipeline frontend without changing its prepared scene, TLAS, material/geometry data, view, output, temporal, or supported-alternate contract.

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
| effect | one renderer operation with one input/output/quality/history contract plus an explicit supported alternate or mandatory-failure policy, such as GBuffer or shadow visibility |
| frontend | thin inline-query or RT-stage code that invokes shared effect semantics |
| pipeline composition | typed stage membership and hit groups, payload/attribute compatibility, recursion, and optional bounded local data for one native RT pipeline; global bindings derive from the selected ray-generation shader |
| logical table plan | Renderer-owned selection and indexing of ray-generation, miss, hit-group, and callable records |
| shader table | RHI-owned backend-materialized GPU records containing identifiers/group handles from one exact native pipeline generation plus bounded local POD |
| pipeline generation | immutable native pipeline version; every compatible table identifies the generation that produced its native identifiers |

The enduring invariants are:

1. One effect owns one public input/output/temporal contract and any supported alternate or mandatory-failure policy regardless of execution mode.
2. Inline and pipeline frontends share semantic kernels and data schemas, never stage intrinsics.
3. One `RenderScene`, prepared scene, TLAS generation, material/geometry identity, and view serve both modes.
4. Renderer policy selects the mode before graph construction; RHI reports capability and performs mechanism.
5. Explicit `Inline` and `Pipeline` requests are strict. Unsupported requests fail with one actionable readiness result before any partial graph is scheduled.
6. `Automatic` may select per effect, but every active mode and reason is stable for the frame and visible in diagnostics/captures.
7. A pipeline is usable only when compiler target, map/library records, runtime API feature chain, stage composition, binding ABI, native pipeline, table, graph path, and selected effect are all ready.
8. A shader-table record can execute only with the exact live pipeline generation from which its identifier/group handle came.
9. TLAS instance contribution, geometry index, ray type, table order, and shader trace parameters follow one documented checked formula.
10. Map/library/pipeline/table/resource generations publish atomically and retire by all-queue submission token, never CPU frame age or device-idle convenience.
11. Classic versus partitioned TLAS and descriptor versus device-address storage never create another shader class, HLSL root, parameter schema, or effect branch. Shaders declare one semantic AS parameter; private RHI lowers it to the selected provider's exact native descriptor representation.

## Non-goals

- Do not emulate ray-generation, miss, hit, intersection, or callable stages inside a generic compute shader.
- Do not force every effect to support both modes or every legal RT stage.
- Do not create an Unreal-scale material, vertex-factory, plugin, or shader-program framework.
- Do not expose D3D12 state objects, Vulkan pipelines, native identifiers/group handles, addresses, strides, or record bytes to Renderer.
- Do not expose TLAS descriptor/device-address representation as a shader class, authored define, effect uniform, graph mode, code variant, or fallback program.
- Do not create a second scene, acceleration-structure, material, history, map, runtime-generation, or effect-settings system.
- Do not advertise pipeline support because compilation or metadata alone succeeds.
- Do not silently fall back from an explicitly requested mode or partially schedule a strict frame.
- Do not rebuild every shader table every frame when its logical content and exact pipeline generation are unchanged.
- Do not put descriptors, owning pointers, transient addresses, variable-size objects, or duplicated material data in local records.
- Do not require any-hit, intersection, callable, recursion, collections, pipeline libraries, GPU-generated tables, permutations, or precaching where a real effect or the explicitly required all-stage evidence does not need them.
- Do not submit permanent test-only shaders, fixtures, executables, registrations, or conformance routes. Use existing validation surfaces or a temporary local harness removed before handoff unless the user separately authorizes submitted test code.

## Current-to-target reconciliation

The revision-pinned current inventory lives in the implementation document. Its architectural meaning is:

```text
CURRENT

typed compute shader -> GlobalShaderMap/code library -> compute pipeline
                                                        |
effect -> frame-graph compute pass -> Dispatch -> RayQuery + shared TLAS

generic six-stage RT route -> typed composition -> neutral/native pipeline + table
                                                -> typed graph TraceRays API
                                                -X no concrete reachable stage consumer yet


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
| RHI public contract | independent AS/inline/pipeline capabilities; one semantic AS resource binding; immutable RT pipeline descriptor; opaque pipeline/table products; logical table materialization request; trace descriptor; states and validation errors | effect/material names, frame scheduling, native handles/bytes, shader access-mode policy |
| D3D12/Vulkan private RHI | selected-provider AS descriptor lowering; native pipeline/state object, layout association, identifier/group-handle retrieval, record packing/alignment, GPU table resources, command encoding, native validation | which material, geometry, ray type, or effect a logical slot means |
| Renderer shader/effect owner | concrete RT shader classes, focused typed composition, shared effect ABI/semantics, requested/active mode, exactly one frontend, output/history/supported-alternate/failure policy | raw identifier bytes, backend table layout, compiler process policy |
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
Readiness and supported alternate or mandatory failure
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
| `Automatic` | no | no | explicit supported alternate or effect failure before graph construction |

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

Acceleration-structure binding representation is not frontend-specific. Inline compute and RT ray generation declare the same semantic scene-AS parameter shape used by their effect. Whether the active scene uses classic TLAS or partitioned TLAS, and whether the native API carries that opaque resource through a descriptor containing a handle or device address, is resolved below shader/effect code. The shader never reconstructs an acceleration structure from address words.

## Shader authoring and pipeline composition

Every RT stage is an ordinary concrete shader class in the same global-shader system, but stage classes expose only the contracts they consume:

- the selected ray-generation shader's nested `Parameters` owns dispatch-global shader-visible input/output fields and is the frame-graph parameter schema;
- miss, hit, intersection, and callable shaders do not repeat that root schema or declare an empty parameter carrier;
- the selected ray-generation shader owns its shared payload, attribute, and recursion compile contract once; the focused composition derives that ABI and never copies it into each participating stage;
- optional local data has one bounded hit-group/stage record schema only where a shader actually consumes it;
- implementation registration owns virtual source, entry/export, and stage;
- the frozen catalog owns immutable type metadata;
- compile jobs and the final map/library own target code and ABI records;
- `ShaderRef<Shader>` owns typed runtime lookup.

`RayTracingPipelineComposition` is the only RT-specific authoring composition. It exists because a native RT pipeline must relate several exports and hit groups. It contains typed identities and hit-group policy—not copied shader names, stages, ABI metadata, code bytes, package names, source strings, native identifiers, graph resources, or effect runtime state. Materialization reads dispatch-wide ABI from the selected ray-generation registration, while neutral export and hit-group descriptors derive local-record contracts from their resolved shader entries rather than accepting another caller-authored copy.

A composition specifies only what is structurally required:

- one or more ray-generation choices, selected one per trace;
- miss shaders ordered by logical ray type;
- triangle/procedural hit groups naming typed closest-hit and optional any-hit/intersection shaders;
- optional callable shaders;
- compatibility with the selected ray-generation shader's authoritative global parameter layout and explicit bounded local layouts when present;
- payload and attribute contracts;
- minimum required recursion policy and debug identity.

Compute and ordinary graphics never use this type. Graphics continues to name concrete vertex/pixel shader refs with the actual draw pipeline description. There is no universal `ShaderProgram`, `TShaderProgram`, pass-registration macro, or string-only export registry.

The graph frontend is `TraceRays<RayGenerationShader>(composition, parameters, dimensions)` with an optional diagnostic-label overload. It mirrors Unreal's separation among global shader class, ray-tracing pipeline initializer, and ray dispatch while hiding Sparkle's materialized pipeline, shader table, native identifiers, global binding writer, addresses, and strides behind the frame-graph/runtime/RHI owners. The exact class and call-site example lives in the [unified authoring document](../../../../../CrossModule/ShaderSystem/README.md#proposed-authoring-experience) so this target contract does not duplicate implementation guidance.

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
| callable | optional composition; focused existing validation or a temporary removed-before-handoff harness proves region/index/ABI/bounds without forcing product use or submitting a test-only fixture |

No product effect adds an empty stage merely to claim coverage. Procedural and callable support may remain conformance-only when no product workload benefits; that limitation is explicit.

### Binding and local data

- The selected ray-generation shader's typed `Parameters` schema is the authoritative global binding layout; other stages do not mirror it.
- Every scene traversal parameter is one semantic acceleration-structure field. Graph setup converts one AS handle through `CreateAccelerationStructureBinding`; it does not use a generic texture/buffer `Read` or pretend the cross-API AS descriptor is an SRV. Private RHI selects and validates the classic/partitioned native descriptor kind and write operation fixed for the active provider before layout/pipeline materialization.
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

The earlier opaque one-ray-type checkpoint intentionally used zero contributions. The current production slice uses nontrivial instance/geometry/two-ray-type indexing through one `RenderRayTracingScene` plan shared by classic and partitioned TLAS. Material/geometry data remains in shared buffers; current product records contain no local data. The delivery plan retains the checkpoint history.

## Capability and readiness contract

Capability is not one `SupportsRayTracing` boolean. The target distinguishes:

- acceleration-structure construction and binding;
- inline ray-query compilation and runtime execution;
- native RT-pipeline compilation, feature/property/function readiness, materialization, shader-table support, command encoding, and typed graph execution.

Pipeline readiness is true only when the full compiler-to-graph chain is usable for the selected target/effect. Extension presence, successful library compilation, generic stage enums, valid metadata, or one backend alone cannot advertise product support.

AS provider readiness is similarly complete-chain truth. A classic or partitioned provider is usable only when construction, resource registration, graph state/lifetime, semantic AS binding, exact native descriptor layout/write, and shader traversal all work together. DirectX binds the same HLSL `RaytracingAccelerationStructure` through native SRV forms; Vulkan defines distinct classic and partitioned acceleration-structure descriptor types, with the PTLAS device address carried by the descriptor write. The Renderer therefore sees neither a device address nor an access-mode enum. If the selected provider's descriptor route is unavailable, provider selection retains another fully supported provider, chooses a real effect-level alternate algorithm when one exists, or fails before graph construction; it does not select another shader representation or fabricate a product.

Provider selection is fixed before binding-layout and pipeline materialization. The backend creates the exact selected-provider descriptor layout; it does not enable or preserve mutable-descriptor machinery solely to switch classic and partitioned AS representations after layout creation.

Shadow visibility is mandatory for direct lighting. Until the pipeline/RGS frontend is complete, inline ray query is the sole real producer and its absence fails before graph construction. Once both frontends exist, the immutable execution plan selects exactly one. A clear, copy, no-op, default texture, stale history, or no-query shader cannot publish the product merely to satisfy graph production.

This boundary follows the [Microsoft DXR resource contract](https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html), [NVIDIA NVRHI semantic acceleration-structure binding](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/doc/ProgrammingGuide.md), and the Khronos [`VK_NV_partitioned_acceleration_structure` descriptor contract](https://docs.vulkan.org/refpages/latest/refpages/source/VK_NV_partitioned_acceleration_structure.html).

This binding choice is not a shader permutation. One `(ShaderTypeId, Target)` code record serves every supported AS provider for that target. There is no `DeviceAddress` shader suffix, authored preprocessor define, hidden backend variant, or second graph path.

Every unavailable result identifies the missing owner-level requirement: compiler target, export/group ABI, backend feature/function, layout/limit, native pipeline/table creation, graph queue/resource rule, generation mismatch, effect frontend, supported alternate, or mandatory producer.

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
- rasterized GBuffer remains an explicit supported algorithm, not a fabricated ray-traced result.

Current source establishes `GBufferAlgorithm::{Rasterized,RayTracing}`, independent `RayTracingExecutionMode::{Automatic,Inline,Pipeline}`, and one immutable graph-construction `RayTracingGBufferExecutionPlan`. `RayTracingGBufferPipeline.hlsl` removes the earlier masked-scene workaround through an any-hit adapter over the same `ResolveRayTracingCandidateAlpha` policy used by `RayQuery`, while opaque groups retain no empty any-hit stage. `DirectShadowSignal` has inline and pipeline frontends over one request/visibility semantic kernel and one pre-graph execution plan; neither route can be replaced by a fabricated product. `RayTracingShaderTablePlan` fixes Surface then ShadowVisibility ordering, checked formula/bounds, material/geometry invalidation, bounded metrics, and the contribution read by both TLAS builders. Graph-owned immutable tables capture the exact pipeline and scene-plan generation and retire with their graph. This is implemented source shape, not native or parity evidence.

The production hit slice adds:

- alpha-tested any-hit as a thin adapter over shared alpha/material policy;
- shadow visibility as a second dual-execution ray type;
- nontrivial instance/geometry/ray-type mapping shared across classic and partitioned TLAS;
- dirty table generation without unnecessary BLAS/TLAS rebuild.

Every remaining current ray-query effect is classified `Dual`, `InlineOnly`, `PipelineOnly`, or `SupportedAlternate`. A `SupportedAlternate` is a real algorithm with its own contract and evidence; shadow visibility has none. Migration requires a useful pipeline design and accepted correctness/quality/history oracle. A global mega-pipeline and forced migration for API coverage are rejected.

## Target completion invariants

The target is realized only when implementation evidence proves all of the following together:

- all six RT stages traverse the final shader/map/library/runtime/graph path in focused D3D12/Vulkan evidence, and any temporary conformance harness is absent from the handoff diff;
- native identifiers/group handles remain private and table regions/indexing/alignment/bounds match the exact pipeline generation;
- ray-traced GBuffer and shadow visibility pass same-frame dual-mode parity including alpha, supported-alternate, and mandatory-failure behavior;
- classic/partitioned TLAS share one logical contribution plan and no scene/material/history authority is duplicated;
- classic/partitioned TLAS use one semantic AS shader/graph binding and private native descriptor lowering; no address/access-mode shader duplicate or fallback shader remains;
- one immutable whole-frame plan implements strict/automatic selection and schedules exactly one frontend per effect;
- map/library/pipeline/table reload and device recreation preserve the previous accepted generation on failure and retire old state by submission token;
- capture/provenance follows shader/effect identity through source, compile job, map/code, composition, native pipeline/table, graph event, and symbols;
- paired correctness, failure, capture, and performance evidence uses fixed hardware/driver/build/scene/camera/settings/sample provenance;
- explicit supported alternate algorithms remain functional, missing mandatory products fail before scheduling, and every single-mode effect is documented honestly;
- no package compatibility, compiler-only RT replacement, ambiguous capability/mode, backend/graph bypass, universal shader-program layer, duplicate owner, permanent migration diagnostics, or unearned precache/permutation framework remains.

The exact implementation prompts, phase ordering, validation sequence, and references are centralized in the [Shader System Delivery Plan](../../../../../../Plans/CrossModule/ShaderSystem.md#implementation-contract). Final proof is centralized in the [Shader System feature acceptance contract](../../../../../CrossModule/ShaderSystem/Acceptance.md).
